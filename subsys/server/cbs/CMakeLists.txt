# ALEX adicionou isto
# Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP)
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories_ifdef(
  CONFIG_CBS
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_include_directories(include)
zephyr_sources_ifdef(CONFIG_CBS cbs.c)
zephyr_sources_ifdef(CONFIG_CBS_LOG cbs_log.c)