from __future__ import annotations

import argparse
import json
import statistics
import subprocess
from pathlib import Path

import matplotlib.pyplot as plt


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run benchmark binary multiple times, save raw JSON to out/, "
            "aggregate timings and build a plot."
        )
    )
    parser.add_argument(
        "--binary",
        type=Path,
        default=Path("build/Release/bench/ThreadOutput"),
        help="Path to benchmark binary.",
    )
    parser.add_argument(
        "--runs",
        type=int,
        default=7,
        help="How many full benchmark runs to execute.",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=Path("out"),
        help="Output directory in project root.",
    )
    return parser.parse_args()


def run_benchmark(binary_path: Path, json_out_path: Path) -> None:
    command = [
        str(binary_path),
        "--benchmark_out_format=json",
        f"--benchmark_out={json_out_path}",
    ]
    subprocess.run(command, check=True)


def extract_series(raw_json_path: Path) -> tuple[dict[int, float], dict[int, float]]:
    with raw_json_path.open("r", encoding="utf-8") as file:
        payload = json.load(file)

    sequential: dict[int, float] = {}
    parallel: dict[int, float] = {}

    for benchmark in payload.get("benchmarks", []):
        name = benchmark.get("name", "")
        if name.endswith(("_mean", "_median", "_stddev")):
            continue

        if "real_time" not in benchmark:
            continue

        arg_value = int(name.split("/")[-1])
        real_time_ns = float(benchmark["real_time"])

        if "BMReportsItemsRecipes" in name:
            sequential[arg_value] = real_time_ns
        elif "BMParallelReportsItemsRecipes" in name:
            parallel[arg_value] = real_time_ns

    return sequential, parallel


def aggregate_series(
    run_files: list[Path],
) -> tuple[list[int], list[float], list[float]]:
    seq_runs: dict[int, list[float]] = {}
    par_runs: dict[int, list[float]] = {}

    for run_file in run_files:
        sequential, parallel = extract_series(run_file)
        for arg_value, time_ns in sequential.items():
            seq_runs.setdefault(arg_value, []).append(time_ns)
        for arg_value, time_ns in parallel.items():
            par_runs.setdefault(arg_value, []).append(time_ns)

    x_values = sorted(set(seq_runs.keys()) & set(par_runs.keys()))
    seq_medians = [statistics.median(seq_runs[x]) for x in x_values]
    par_medians = [statistics.median(par_runs[x]) for x in x_values]
    return x_values, seq_medians, par_medians


def build_plot(
    x_values: list[int], seq_ns: list[float], par_ns: list[float], out_dir: Path
) -> Path:
    plt.figure(figsize=(10, 6))
    plt.plot(x_values, seq_ns, marker="o", label="ReportsItems (sequential)")
    plt.plot(x_values, par_ns, marker="o", label="ParallelReportsItems")
    plt.xlabel("Recipes count")
    plt.ylabel("Real time (ns)")
    plt.title("Benchmark: ReportsItems vs ParallelReportsItems")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()

    plot_path = out_dir / "thread_output_time.png"
    plt.savefig(plot_path, dpi=140)
    plt.close()
    return plot_path


def main() -> None:
    args = parse_args()
    binary_path = args.binary.resolve()
    out_dir = args.out_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    if not binary_path.exists():
        raise FileNotFoundError(f"Benchmark binary not found: {binary_path}")

    run_files: list[Path] = []
    for run_index in range(1, args.runs + 1):
        run_file = out_dir / f"thread_output_run_{run_index:02d}.json"
        run_benchmark(binary_path, run_file)
        run_files.append(run_file)

    x_values, seq_ns, par_ns = aggregate_series(run_files)
    if not x_values:
        raise RuntimeError("No benchmark data found in JSON output.")

    summary_path = out_dir / "thread_output_summary.csv"
    with summary_path.open("w", encoding="utf-8") as file:
        file.write(
            "recipes_count,reports_items_median_ns,parallel_reports_items_median_ns\n"
        )
        for recipes_count, seq_time, par_time in zip(x_values, seq_ns, par_ns):
            file.write(f"{recipes_count},{seq_time:.3f},{par_time:.3f}\n")

    plot_path = build_plot(x_values, seq_ns, par_ns, out_dir)
    print(f"Raw runs: {len(run_files)} file(s) in {out_dir}")
    print(f"Summary: {summary_path}")
    print(f"Plot: {plot_path}")


if __name__ == "__main__":
    main()
