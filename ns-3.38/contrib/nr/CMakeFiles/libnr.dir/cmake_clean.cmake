file(REMOVE_RECURSE
  "../../build/lib/libns3.38-nr-default.pdb"
  "../../build/lib/libns3.38-nr-default.so"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/libnr.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
