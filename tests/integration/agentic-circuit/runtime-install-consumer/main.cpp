#include "gfsim/object.h"

int main() {
  gfsim::SimSystem system("installed-runtime");
  system.requestTerminate(gfsim::TerminationClass::Completed, "consumer_done");
  const gfsim::TerminationResult result = system.run();
  return result.classification == gfsim::TerminationClass::Completed ? 0 : 1;
}
