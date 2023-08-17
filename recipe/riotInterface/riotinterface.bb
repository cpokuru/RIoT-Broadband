LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

SRC_URI = "git://github.com/cpokuru/RIoT-Broadband.git;protocol=https;branch=main"
# Modify these as desired
#PV = "1.0+git${SRCPV}"
#SRCREV = "${AUTOREV}"
SRCREV = "95f8bb375b8f1a8d8021c8180a901c555baa9221"
DEPENDS = "avahi rbus"
S = "${WORKDIR}/git"

inherit cmake

# Specify any options you want to pass to cmake using EXTRA_OECMAKE:
EXTRA_OECMAKE = ""
FILES_${PN} = "${bindir}/*"

