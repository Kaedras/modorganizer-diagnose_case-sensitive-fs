cmake_minimum_required(VERSION 3.30)

add_library(diagnose_case-sensitive-fs SHARED)
mo2_configure_plugin(diagnose_case-sensitive-fs WARNINGS OFF)
mo2_install_target(diagnose_case-sensitive-fs)
