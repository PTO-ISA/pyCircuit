#include "parallel_gfsim/SimTestbench.h"

namespace parallel_gfsim {

void SimTestbench::Work()
{
    DriveStimuli();
    WorkChildren();
    CheckOutputs();
    if (Done() && !GetSim()->NeedTerminate()) {
        GetSim()->RequestTerminate(Name() + " complete");
    }
}

void SimTestbench::Xfer()
{
    AdvanceInnerQueues();
    XferChildren();
}

void SimTestbench::Reset()
{
    stimuliSent_ = 0;
    outputsChecked_ = 0;
    ResetBench();
    ResetInnerQueues();
    ResetChildren();
}

void SimTestbench::ReportStat()
{
    auto& report = Report();
    report.ReportTitle(Name() + " Testbench");
    report.ReportVal("Stimuli sent", stimuliSent_);
    report.ReportVal("Outputs checked", outputsChecked_);
    ReportBench();
    ReportChildren();
}

void SimTestbench::RecordStimulus(const std::string& label)
{
    ++stimuliSent_;
    GetSim()->MarkProgress();
    Log().Debug(Name(), "stimulus") << label;
}

void SimTestbench::RecordOutputChecked(const std::string& label)
{
    ++outputsChecked_;
    GetSim()->MarkProgress();
    Log().Debug(Name(), "check") << label;
}

} // namespace parallel_gfsim
