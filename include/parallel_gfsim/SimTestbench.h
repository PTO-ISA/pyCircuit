#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "SimAssert.h"
#include "SimModule.h"

namespace parallel_gfsim {

// Port-agnostic stimulus driver and output checker.
class SimTestbench : public SimModule {
public:
    explicit SimTestbench(std::string name, SimSystem* system = nullptr)
        : SimModule(std::move(name), system) {}

    void Work() final;
    void Xfer() final;
    void Reset() final;
    void ReportStat() final;

    std::uint64_t StimuliSent() const { return stimuliSent_; }
    std::uint64_t OutputsChecked() const { return outputsChecked_; }

protected:
    virtual void DriveStimuli() = 0;
    virtual void CheckOutputs() = 0;
    virtual bool Done() const = 0;
    virtual void ResetBench() = 0;
    virtual void ReportBench() {}

    void RecordStimulus(const std::string& label);
    void RecordOutputChecked(const std::string& label);

    template <typename T>
    void ExpectEqual(const T& actual, const T& expected,
                     const std::string& label)
    {
        PARALLEL_GFSIM_ASSERT(actual == expected) << Name() << ' ' << label;
        RecordOutputChecked(label);
    }

private:
    std::uint64_t stimuliSent_ = 0;
    std::uint64_t outputsChecked_ = 0;
};

} // namespace parallel_gfsim
