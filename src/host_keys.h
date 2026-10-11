#pragma once
#include "libssh_esp32.h"
#include <libssh/libssh.h>

namespace HostKeys {
  ssh_known_hosts_e check(ssh_session session);
  bool remember(ssh_session session);
}
