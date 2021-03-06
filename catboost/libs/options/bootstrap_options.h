#pragma once

#include "enums.h"
#include "json_helper.h"
#include <catboost/libs/helpers/exception.h>
#include <catboost/libs/logging/logging.h>
#include <util/generic/yexception.h>
#include <cmath>

namespace NCatboostOptions {
    class TBootstrapConfig {
    public:
        explicit TBootstrapConfig()
            : TakenFraction("sample_rate", 0.66)
            , BaggingTemperature("bagging_temperature", 1.0)
            , BootstrapType("type", EBootstrapType::Bayesian)
        {
        }

        float GetTakenFraction() const {
            return TakenFraction.Get();
        }

        float GetPoissonLambda() const {
            const float takenFraction = TakenFraction.Get();
            return takenFraction < 1 ? -log(1 - takenFraction) : -1;
        }

        EBootstrapType GetBootstrapType() const {
            return BootstrapType.Get();
        }

        float GetBaggingTemperature() const {
            return BaggingTemperature.Get();
        }

        void Validate() const;

        void Load(const NJson::TJsonValue& options) {
            CheckedLoad(options, &TakenFraction, &BaggingTemperature, &BootstrapType);
        }

        void Save(NJson::TJsonValue* options) const {
            switch (BootstrapType.Get()) {
                case EBootstrapType::Bayesian: {
                    SaveFields(options, BaggingTemperature, BootstrapType);
                    break;
                }
                case EBootstrapType::No: {
                    SaveFields(options, BootstrapType);
                }
                default: {
                    SaveFields(options, TakenFraction, BootstrapType);
                    break;
                }
            }
        }

        bool operator==(const TBootstrapConfig& rhs) const {
            return std::tie(TakenFraction, BaggingTemperature, BootstrapType) ==
                   std::tie(rhs.TakenFraction, rhs.BaggingTemperature, rhs.BootstrapType);
        }

        bool operator!=(const TBootstrapConfig& rhs) const {
            return !(rhs == *this);
        }

    private:
        TOption<float> TakenFraction;
        TOption<float> BaggingTemperature;
        TOption<EBootstrapType> BootstrapType;
    };

}
