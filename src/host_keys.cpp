#include "host_keys.h"
#include "fsx.h"

namespace HostKeys {
static String path() { return fsx::usingSD() ? "/deckssh/known_hosts" : "/known_hosts"; }
static constexpr size_t maxLine = 8192;

ssh_known_hosts_e check(ssh_session session) {
  char* exported = nullptr;
  if (ssh_session_export_known_hosts_entry(session, &exported) != SSH_OK)
    return SSH_KNOWN_HOSTS_ERROR;
  String host(exported);
  ssh_string_free_char(exported);
  const int space = host.indexOf(' ');
  if (space <= 0) return SSH_KNOWN_HOSTS_ERROR;
  host.remove(space); // libssh's canonical hostname, including non-default port

  ssh_key serverKey = nullptr;
  if (ssh_get_server_publickey(session, &serverKey) != SSH_OK)
    return SSH_KNOWN_HOSTS_ERROR;
  fs::FS& disk = fsx::fs();
  const String filename = path();
  if (!disk.exists(filename)) {
    ssh_key_free(serverKey);
    return SSH_KNOWN_HOSTS_NOT_FOUND;
  }
  File file = disk.open(filename, "r");
  ssh_known_hosts_e result = SSH_KNOWN_HOSTS_UNKNOWN;
  if (!file || file.isDirectory()) result = SSH_KNOWN_HOSTS_ERROR;
  while (file && file.position() < file.size() && result != SSH_KNOWN_HOSTS_OK &&
         result != SSH_KNOWN_HOSTS_ERROR) {
    String line;
    while (file.position() < file.size()) {
      const int c = file.read();
      if (c < 0 || line.length() >= maxLine) { result = SSH_KNOWN_HOSTS_ERROR; break; }
      if (c == '\n') break;
      line += char(c);
    }
    if (result == SSH_KNOWN_HOSTS_ERROR) break;
    line.trim();
    if (line.isEmpty() || line[0] == '#') continue;
    // Match OpenSSH host patterns/ports and compare actual public keys using
    // libssh's parser; use Arduino FS for the file I/O on SD and LittleFS.
    // Unsupported marker entries must not silently establish trust.
    if (line[0] == '@') { result = SSH_KNOWN_HOSTS_ERROR; break; }
    ssh_knownhosts_entry* entry = nullptr;
    const int rc = ssh_known_hosts_parse_line(host.c_str(), line.c_str(), &entry);
    if (rc == SSH_AGAIN) continue; // another host
    if (rc != SSH_OK || !entry || !entry->publickey) {
      ssh_knownhosts_entry_free(entry);
      result = SSH_KNOWN_HOSTS_ERROR;
      break;
    }
    if (ssh_key_cmp(serverKey, entry->publickey, SSH_KEY_CMP_PUBLIC) == 0)
      result = SSH_KNOWN_HOSTS_OK;
    else if (ssh_key_type(serverKey) == ssh_key_type(entry->publickey))
      result = SSH_KNOWN_HOSTS_CHANGED;
    else if (result != SSH_KNOWN_HOSTS_CHANGED)
      result = SSH_KNOWN_HOSTS_OTHER;
    ssh_knownhosts_entry_free(entry);
  }
  file.close();
  ssh_key_free(serverKey);
  return result;
}

bool remember(ssh_session session) {
  if (check(session) == SSH_KNOWN_HOSTS_OK) return true;
  char* exported = nullptr;
  if (ssh_session_export_known_hosts_entry(session, &exported) != SSH_OK) return false;
  fs::FS& disk = fsx::fs();
  const String filename = path();
  bool needsNewline = false;
  if (disk.exists(filename)) {
    File previous = disk.open(filename, "r");
    if (!previous || previous.isDirectory()) { ssh_string_free_char(exported); return false; }
    if (previous.size()) {
      if (!previous.seek(previous.size() - 1)) { ssh_string_free_char(exported); return false; }
      const int last = previous.read();
      if (last < 0) { ssh_string_free_char(exported); return false; }
      needsNewline = last != '\n';
    }
    previous.close();
  }
  File file = disk.open(filename, "a");
  const size_t length = strlen(exported);
  const bool written = file && (!needsNewline || file.write('\n') == 1) &&
      file.write(reinterpret_cast<const uint8_t*>(exported), length) == length;
  file.flush();
  file.close();
  ssh_string_free_char(exported);
  // Never claim "Accept & save" succeeded until this session's key matches
  // an entry read back through the same path used on the next connection.
  return written && check(session) == SSH_KNOWN_HOSTS_OK;
}
}
