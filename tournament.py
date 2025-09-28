from argparse import ArgumentParser
from pathlib import Path
import os
import subprocess
import sys

def usable_cpus() -> int:
    return len(os.sched_getaffinity(0))

USABLE_CPUS: int = usable_cpus()
ORDO_PROG: str = 'ordo'
BLUNDER_PROG: str = 'blunder-7.6.0'
LEORIK_PROG: str = 'leorik-2.0.2'
SHENYU_PROG: str = 'shenyu_2.0.1'
ADMETE_PROG: str = 'admete_1.3.0_linux_x86-64'
OPPONENTS: list[str] = [BLUNDER_PROG, LEORIK_PROG, SHENYU_PROG, ADMETE_PROG]
OPP_ROUNDS: int = 1000

def clear_pgns() -> None:
    resp = input("Remove old PGNs? [Y/n]")
    if resp.lower() != 'y':
        print("Will not remove old PGNs.")
        return
    print("Clearing old PGNs...")
    for opponent in OPPONENTS:
        pgn: Path = Path(f'vs_{opponent}.pgn')
        if pgn.is_file():
            os.remove(pgn)

def sh_run(*args, **kwargs):
    ret: int = subprocess.run(*args, **kwargs).returncode
    if ret != 0:
        raise RuntimeError(f"subprocess.run (with args = {args}, kwargs = {kwargs}) returned {ret}")

def download_blunder() -> None:
    prog: Path = Path(BLUNDER_PROG)
    if not prog.is_file():
        zip_url: str = 'https://github.com/deanmchris/blunder/releases/download/v7.6.0/blunder-7.6.0.zip'
        sh_run(['wget', zip_url])
        zip_name: str = zip_url.split('/')[-1]
        zip_dest: str = 'blunder'
        sh_run(['unzip', zip_name, '-d', zip_dest])
        sh_run(['rm', zip_name])
        sh_run(['cp', f'{zip_dest}/blunder-7.6.0/linux/blunder-linux', BLUNDER_PROG])
        sh_run(['rm', '-rf', zip_dest])
        sh_run(['chmod', '+x', BLUNDER_PROG])

def download_leorik() -> None:
    prog: Path = Path(LEORIK_PROG)
    if not prog.is_file():
        zip_url: str = 'https://github.com/lithander/Leorik/releases/download/2.0/Leorik.2.0.Linux.zip'
        sh_run(['wget', zip_url])
        zip_name: str = zip_url.split('/')[-1]
        zip_dest: str = 'leorik'
        sh_run(['unzip', zip_name, '-d', zip_dest])
        sh_run(['rm', zip_name])
        sh_run(['cp', f'{zip_dest}/Leorik 2.0 Linux/Leorik-2.0.2', LEORIK_PROG])
        sh_run(['rm', '-rf', zip_dest])
        sh_run(['chmod', '+x', LEORIK_PROG])

def download_shenyu() -> None:
    prog: Path = Path(SHENYU_PROG)
    if not prog.is_file():
        prog_url: str = 'https://github.com/AAce3/ShenYu/releases/download/2.0.1/shenyu_v2.0.0_x86_64_linux'
        sh_run(['wget', prog_url])
        prog_name: str = prog_url.split('/')[-1]
        sh_run(['mv', prog_name, SHENYU_PROG])
        sh_run(['chmod', '+x', SHENYU_PROG])

def download_admete() -> None:
    prog: Path = Path(ADMETE_PROG)
    if not prog.is_file():
        prog_url: str = 'https://github.com/orbita2d/admete/releases/download/v1.3.0/admete_1.3.0_linux_x86-64'
        sh_run(['wget', prog_url])
        sh_run(['chmod', '+x', ADMETE_PROG])

def download_opponents() -> None:
    download_blunder()
    download_leorik()
    download_shenyu()
    download_admete()

def run_vs(opponent: str) -> None:
    cmd: list[str] = []
    cmd.extend(['./runner'])
    cmd.extend(['-engine', 'name=tuna', 'cmd=./all_tests2'])
    cmd.extend(['-engine', f'name={opponent}', f'cmd=./{opponent}'])
    cmd.extend(['-each', 'tc=10+0.1', 'proto=uci', 'option.Hash=16'])
    cmd.extend(['-recover', '-repeat', '-rounds', str(OPP_ROUNDS)])
    cmd.extend(['-openings', 'file=8moves_v3.pgn', 'format=pgn'])
    cmd.extend(['-pgnout', f'vs_{opponent}.pgn'])
    cmd.extend(['-concurrency', str(USABLE_CPUS)])
    cmd.extend(['-config']);
    #cmd.extend(['|', 'tee', f'vs_{opponent}.log'])
    sh_run(cmd)

def calculate_elo() -> None:
    prog: Path = Path(ORDO_PROG)
    if not prog.is_file():
        ordo_path: str = 'https://github.com/michiguel/Ordo/releases/download/1.0/ordo-linux64'
        sh_run(['wget', ordo_path])
        ordo_name: str = ordo_path.split('/')[-1]
        sh_run(['mv', ordo_name, ORDO_PROG])
        sh_run(['chmod', '+x', ORDO_PROG])

    cmd: list[str] = ['cat']
    for opponent in OPPONENTS:
        cmd.append(f'vs_{opponent}.pgn')
    with open('all.pgn', 'w') as pgn:
        sh_run(cmd, stdout=pgn)
    cmd = ['./ordo', '-m', 'anchors.txt', '-p', 'all.pgn']
    sh_run(cmd)

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('--calculate', action='store_true',
                        help="Only calculates ELO from existing PGNs. No games are played.")
    parser.add_argument('--concurrency', type=int, default=None,
                        help="Number of concurrent threads for runner")
    args = parser.parse_args()
    if args.calculate:
        calculate_elo()
        sys.exit(0)
    if args.concurrency is not None:
        USABLE_CPUS = args.concurrency
    clear_pgns()
    download_opponents()
    for opponent in OPPONENTS:
        run_vs(opponent)
    calculate_elo()
