hunter_config(bcos-framework VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/d3358f990cb9040b76abe4a8f9fd9d4f4e9f9f71.tar.gz
    SHA1 33c2e4f31827fda835e1eee6ecc84a4c28743d0d
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON #DEBUG=ON
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/56aeb750885c7820e8c6d6bd6fbd6e8d579dc8b8.tar.gz
    SHA1 d91b398fdcc0165f96e475d2a475651e7d138294
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON URL_BASE=${URL_BASE}
)