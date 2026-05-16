add_library(VelaUtility INTERFACE)
target_include_directories(VelaUtility INTERFACE include)
add_library(Vela::Utility ALIAS VelaUtility)