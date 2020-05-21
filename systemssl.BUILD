package(default_visibility = ["//visibility:public"])
cc_library(
    name = "openssl",
    hdrs = glob(["**/openssl/*.h"]),
    srcs = glob([
        "**/libssl.dylib",
        "**/libcrypto.dylib",
    ]),
    includes = [
        "include/",
    ],
    visibility = ["//visibility:public"],
)
