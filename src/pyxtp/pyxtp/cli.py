"""Copyright 2009-2020 The VOTCA Development Team (http://www.votca.org)

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 Module to read user input and perform the requested input action."""

import argparse
import logging
from pathlib import Path

import pkg_resources


logger = logging.getLogger(__name__)

VERSION = pkg_resources.get_distribution('pyxtp').version


def exists(input_file: str) -> Path:
    """Check if the input file exists."""
    path = Path(input_file)
    if not path.exists():
        raise argparse.ArgumentTypeError(f"{input_file} doesn't exist!")

    return path


def configure_logger(workdir: Path) -> None:
    """Set the logging infrasctucture."""
    file_log = workdir / 'moka_output.log'
    logging.basicConfig(filename=file_log, level=logging.INFO,
                        format='%(asctime)s  %(message)s',
                        datefmt='[%I:%M:%S]')
    handler = logging.StreamHandler()
    handler.terminator = ""

    path = pkg_resources.resource_filename('moka', '')

    logger.info(f"\nUsing pyxtp version: {VERSION}\n")
    logger.info(f"pyxtp path is: {path}\n")
    logger.info(f"Working directory is: {workdir.absolute().as_posix()}\n")


def parse_user_arguments() -> argparse.Namespace:
    """Read the user arguments."""
    parser = argparse.ArgumentParser("xtp_cli")
    parser.add_argument('--version', action='version',
                        version=f"%(prog)s {VERSION}")

    # Common arguments to all calculators
    executables = ["eanalyze", "einternal", "ianalyze",
                   "kmclifetime", "kmcmultiple", "mapchecker", "neighborlist"]
    parser.add_argument(
        '-e', '--executable', required=True, help="Name of the calculator to run", choices=executables)
    parser.add_argument('-f', '--file', help="hdf5 state file, .hdf5")
    parser.add_argument(
        '-i', '--first_frame', default=0, help="start from this frame")
    parser.add_argument(
        '-n', '--nframes', default=1, help="number of frames to process")
    parser.add_argument(
        '-t', '--nthreads', default=1, help="number of threads to create")
    parser.add_argument(
        '-s', '--save', help="whether or not to save changes to state file", action="store_true")

    # Read the arguments
    return parser.parse_args()


def main():
    """Parse the command line arguments to compute or query data from the database."""
    opts = parse_user_arguments()


if __name__ == "__main__":
    main()
