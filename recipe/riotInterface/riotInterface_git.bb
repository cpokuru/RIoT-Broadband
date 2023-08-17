LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=b8674f44703ede95bec14083251335db"

SRC_URI = "https://github.com/cpokuru/RIoT-Broadband.git;protocol=https;branch=main"
SRC_URI[sha256sum] = "0000000000000000000000000000000000000000000000000000000000000000"

# Modify these as desired
PV = "1.0+git${SRCPV}"
SRCREV = "${AUTOREV}"
DEPENDS = "rbus"
S = "${WORKDIR}/git"

inherit cmake

# Specify any options you want to pass to cmake using EXTRA_OECMAKE:
EXTRA_OECMAKE = ""
FILES_${PN} = "${bindir}/*"
