# Mostly copied from https://github.com/sblantipodi/platformio_version_increment but modified
# https://github.com/sblantipodi/platformio_version_increment/blob/781207e2d24c4cfcc274a4f5aa8529741e129697/version_increment_pre.py

import datetime
import os

import os
import subprocess
from datetime import datetime

def read_version_file(path="version.txt"):
  if os.path.isfile(path):
    with open(path, "r") as f:
      return f.read().strip()
  return "0.1.0"

def get_git_branch():
  try:
    return subprocess.check_output(
      ["git", "rev-parse", "--abbrev-ref", "HEAD"],
      stderr=subprocess.DEVNULL
    ).decode().strip()
  except subprocess.CalledProcessError:
    return "unknown"

def get_git_short_sha():
  try:
    full_sha = subprocess.check_output(
      ["git", "rev-parse", "HEAD"],
      stderr=subprocess.DEVNULL
    ).decode().strip()
    return full_sha[:7]
  except subprocess.CalledProcessError:
    return "0000000"

def get_build_timestamp():
  return (
    os.environ.get("GITVERSION_BUILDDATE") or
    os.environ.get("BUILD_TIMESTAMP") or
    datetime.utcnow().strftime("%Y-%m-%d")
  )

def is_dogfood():
  return not any([
    os.environ.get("CI"),
    os.environ.get("GITHUB_ACTIONS"),
    os.environ.get("GITLAB_CI"),
    os.environ.get("JENKINS_HOME"),
    os.environ.get("BUILD_ID")
  ])

def get_version():
  # If GitVersion provides full semver, we use it as-is (assumed complete).
  env_version = os.environ.get("GITVERSION_FULLSEMVER") or os.environ.get("VERSION")
  if env_version:
    return env_version

  base_version = read_version_file()
  branch = os.environ.get("GITVERSION_ESCAPEDBRANCHNAME") or get_git_branch()
  sha = os.environ.get("GITVERSION_SHORTSHA") or get_git_short_sha()
  version = f"{base_version}+{branch}.{sha}"

  if is_dogfood():
    version += ".dogfood"

  return version

def write_version_header(version, timestamp, output_path="include/Version.h"):
  os.makedirs(os.path.dirname(output_path), exist_ok=True)
  with open(output_path, "w") as f:
    f.write("// Auto-generated file, do not edit\n\n")
    f.write("#ifndef VERSION_H // VERSION_H\n\n")
    f.write(f'#define DEVICE_SOFTWARE_VERSION "{version}"\n')
    f.write(f'#define BUILD_TIMESTAMP "{timestamp}"\n')
    f.write("\n#endif // VERSION_H\n")

version = get_version()
timestamp = get_build_timestamp()
write_version_header(version, timestamp)
print(f"Generated include/Version.h with VERSION={version} and BUILD_TIMESTAMP={timestamp}")
