// The only PawCore file that includes an engine header: the module glue UBT needs. The standalone test build
// (tools/build_pawcore_tests.ps1) leaves this file out.
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, PawCore);
