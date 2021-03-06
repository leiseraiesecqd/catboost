#pragma once

#include <catboost/libs/options/overfitting_detector_options.h>
#include <catboost/libs/helpers/exception.h>
#include <catboost/libs/logging/logging.h>

#include <util/generic/deque.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>

#include <library/logger/global/global.h>


class IOverfittingDetector {
public:
    virtual ~IOverfittingDetector() {
    }
    virtual void AddError(double err) = 0;
    virtual bool IsNeedStop() const = 0;
    virtual int GetIterationsWait() const = 0;
    virtual double GetCurrentPValue() const = 0;
    virtual double GetThreshold() const = 0;
    virtual bool GetMaxIsOptimal() const = 0;
    virtual bool IsActive() const = 0;
};

class TOverfittingDetectorBase : public IOverfittingDetector {
public:
    TOverfittingDetectorBase(bool maxIsOptimal, double threshold, int iterationsWait)
        : IsEmpty(true)
        , Threshold(threshold)
        , MaxIsOptimal(maxIsOptimal)
        , IterationsWait(iterationsWait)
        , CurrentPValue(1.0) {
    }

    int GetIterationsWait() const override {
        return IterationsWait;
    }

    double GetCurrentPValue() const override {
        return CurrentPValue;
    }

    double GetThreshold() const override {
        return Threshold;
    }

    bool GetMaxIsOptimal() const override {
        return MaxIsOptimal;
    }

    bool IsNeedStop() const override {
        return (!IsEmpty) && (CurrentPValue < Threshold);
    }

    bool IsActive() const override {
        return Threshold > 0;
    }

protected:
    bool IsEmpty;

    const double Threshold;
    const bool MaxIsOptimal;
    const int IterationsWait;

    double CurrentPValue;
};

class TOverfittingDetectorWilcoxon : public TOverfittingDetectorBase {
public:
    TOverfittingDetectorWilcoxon(bool maxIsOptimal, double threshold, int iterationsWait, bool hasTest)
        : TOverfittingDetectorBase(maxIsOptimal, hasTest ? threshold : 0, iterationsWait) {
        CB_ENSURE(hasTest || threshold == 0, "No test provided, cannot check overfitting.");
    }

    void AddError(double err) override;

private:
    void UpdatePValue();

    TVector<double> DeltasAfterLocalMax;
    double LastError;
    double LocalMax;
};

class TOverfittingDetectorIncToDec : public TOverfittingDetectorBase {
public:
    TOverfittingDetectorIncToDec(bool maxIsOptimal, double threshold, int iterationsWait, bool hasTest)
        : TOverfittingDetectorBase(maxIsOptimal, hasTest ? threshold : 0, iterationsWait) {
    }

    void AddError(double err) override;

private:
    const double LAMBDA_FORGET = 0.99;
    const int ITERATION_FORGET = 2000;
    const double LAMBDA_SCALE = 0.5;
    const double EPS = 1e-10;

    void UpdatePValue();

    TDeque<double> Errors;

    double LocalMax;
    double ExpectedInc;
    double LastError;

    int IterationsFromLocalMax;
};

inline bool NeedOverfittingDetection(const IOverfittingDetector* detector) {
    return (detector->GetThreshold() > 0.0);
}

inline bool DetectOverfitting(double testError,
                              IOverfittingDetector* detector,
                              TVector<double>* valuesToLog) {
    detector->AddError(testError);
    double pValue = detector->GetCurrentPValue();
    valuesToLog->push_back(pValue);
    return detector->IsNeedStop();
}

inline THolder<IOverfittingDetector> CreateOverfittingDetector(EOverfittingDetectorType type,
                                                                bool maxIsOptimal,
                                                                double threshold,
                                                                int iterationsWait,
                                                                bool hasTest) {
    switch (type)
    {
        case EOverfittingDetectorType::IncToDec:
        {
            return MakeHolder<TOverfittingDetectorIncToDec>(maxIsOptimal, threshold, iterationsWait, hasTest);
        }
        case EOverfittingDetectorType::Iter:
        {
            return MakeHolder<TOverfittingDetectorIncToDec>(maxIsOptimal, 1.0, iterationsWait, hasTest);
        }
        case EOverfittingDetectorType::Wilcoxon:
        {
            return MakeHolder<TOverfittingDetectorWilcoxon>(maxIsOptimal, threshold, iterationsWait, hasTest);
        }
        default:
        {
            CB_ENSURE(false, "Unknown OD type: " << type);
        }
    }
}

inline THolder<IOverfittingDetector> CreateOverfittingDetector(const NCatboostOptions::TOverfittingDetectorOptions& options, bool maxIsOptimal, bool hasTest) {
    return CreateOverfittingDetector(options.OverfittingDetectorType, maxIsOptimal, options.AutoStopPValue, options.IterationsWait, hasTest);
}
