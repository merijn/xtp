file(GLOB_RECURSE VOTCA_XTP_XML *.xml)
install(FILES ${VOTCA_XTP_XML} DESTINATION ${CMAKE_INSTALL_DATADIR}/votca/xtp/xml)
add_subdirectory(subpackages)