# Copyright (c) 2022 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only

set(base_examples
    cttc-3gpp-channel-example
    cttc-3gpp-channel-simple-ran
    cttc-3gpp-channel-simple-fdm
    cttc-3gpp-indoor-calibration
    cttc-3gpp-channel-nums
    cttc-3gpp-channel-nums-fdm
    cttc-nr-cc-bwp-demo
    cttc-lte-ca-demo
    cttc-nr-demo
    cttc-error-model
    cttc-error-model-amc
    cttc-error-model-comparison
    cttc-channel-randomness
    rem-example
    rem-beam-example
    cttc-fh-compression
    cttc-nr-notching
    cttc-nr-mimo-demo
    cttc-nr-traffic-ngmn-mixed
    cttc-nr-traffic-3gpp-xr
    traffic-generator-example
)
foreach(
  example
  ${base_examples}
)
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK ${libnr}
                      ${libflow-monitor}
  )
endforeach()


set(lena-lte-comparison_examples
    lena-lte-comparison-user
    lena-lte-comparison-campaign
)
set(lena-lte-comparison_source_files
    lena-lte-comparison/lena-lte-comparison.cc
    lena-lte-comparison/sinr-output-stats.cc
    lena-lte-comparison/flow-monitor-output-stats.cc
    lena-lte-comparison/power-output-stats.cc
    lena-lte-comparison/slot-output-stats.cc
    lena-lte-comparison/lena-v1-utils.cc
    lena-lte-comparison/lena-v2-utils.cc
    lena-lte-comparison/rb-output-stats.cc
)
foreach(
  example
  ${lena-lte-comparison_examples}
)
  build_lib_example(
    NAME ${example}
    SOURCE_FILES lena-lte-comparison/${example}.cc
                 ${lena-lte-comparison_source_files}
    LIBRARIES_TO_LINK ${libnr}
                      ${libflow-monitor}
                      ${libstats}
  )
endforeach()

set(3gpp-outdoor-calibration_source_files
    3gpp-outdoor-calibration/cttc-nr-3gpp-calibration.cc
    3gpp-outdoor-calibration/sinr-output-stats.cc
    3gpp-outdoor-calibration/flow-monitor-output-stats.cc
    3gpp-outdoor-calibration/power-output-stats.cc
    3gpp-outdoor-calibration/slot-output-stats.cc
    3gpp-outdoor-calibration/rb-output-stats.cc
    3gpp-outdoor-calibration/cttc-nr-3gpp-calibration-utils-v1.cc
    3gpp-outdoor-calibration/cttc-nr-3gpp-calibration-utils-v2.cc
)

build_lib_example(
    NAME cttc-nr-3gpp-calibration-user
    SOURCE_FILES 3gpp-outdoor-calibration/cttc-nr-3gpp-calibration-user.cc
                 ${3gpp-outdoor-calibration_source_files}
    LIBRARIES_TO_LINK ${libnr}
                      ${libflow-monitor}
                      ${libstats}
)

set(example cttc-realistic-beamforming)
set(source_files ${example}.cc)
set(libraries_to_link ${libnr} ${libflow-monitor} ${SQLite3_LIBRARIES})
build_lib_example(
  NAME ${example}
  SOURCE_FILES ${source_files}
  LIBRARIES_TO_LINK ${libraries_to_link}
)
