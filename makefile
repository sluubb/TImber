# ----------------------------
# Makefile Options
# ----------------------------

NAME = TIMBER
ICON = icon.png
DESCRIPTION = "First person game prototype for the TI-84 Plus CE."
COMPRESSED = NO
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
