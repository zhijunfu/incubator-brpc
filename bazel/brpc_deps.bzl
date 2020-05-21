def brpc_deps():
     native.new_local_repository(
           name = "systemssl",
           path = "/usr/local/opt/openssl",
           build_file = "@com_github_brpc_brpc//:systemssl.BUILD",
     )
