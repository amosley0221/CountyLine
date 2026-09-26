"""Summarize a County Line benchmark capture.

Reads frames.csv (and run.json for metadata, when present) from a capture folder
and reports sample count, elapsed time, mean FPS, median/p95/p99 frame
milliseconds and counts of slow frames. This parser is the authority for reported
numbers; the in-game JSON is a convenience copy of the same definitions.

Definitions (also in Docs/PERFORMANCE_BENCHMARK.md):
  * Warm-up rows (phase != "measured") are dropped before anything is computed.
  * elapsed_seconds is the sum of measured frame durations.
  * mean_fps is measured frames / elapsed_seconds. Per-frame FPS is never averaged.
  * Percentiles use nearest-rank on durations sorted ascending:
    index = ceil(P/100 * N) - 1, clamped to [0, N-1].
  * median is the middle value (odd N) or the mean of the two middle values (even N).
  * Threshold counts are frames strictly greater than 33.33 / 50 / 100 ms.
  * Optional columns that are absent or blank stay unavailable (None), never 0.

Usage:
  python summarize_benchmark.py <capture folder or frames.csv> [--json out.json]
Standard library only.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import sys
from pathlib import Path

THRESHOLDS = (33.33, 50.0, 100.0)
MEASURED_PHASE = 'measured'


class BenchmarkError(Exception):
    """Raised when a capture cannot be summarized at all."""


def percentile(sorted_ms: list[float], percent: float) -> float:
    """Nearest-rank percentile over an ascending sorted list."""
    if not sorted_ms:
        raise BenchmarkError('no samples')
    rank = math.ceil(percent / 100.0 * len(sorted_ms))
    return sorted_ms[min(max(rank - 1, 0), len(sorted_ms) - 1)]


def median(sorted_ms: list[float]) -> float:
    if not sorted_ms:
        raise BenchmarkError('no samples')
    middle = len(sorted_ms) // 2
    if len(sorted_ms) % 2:
        return sorted_ms[middle]
    return (sorted_ms[middle - 1] + sorted_ms[middle]) / 2.0


def _optional_float(row: dict, column: str) -> float | None:
    """A blank or missing optional metric is unavailable, not zero."""
    raw = (row.get(column) or '').strip()
    if not raw:
        return None
    try:
        return float(raw)
    except ValueError:
        return None


def read_frames(csv_path: Path) -> tuple[list[dict], list[str]]:
    """Return measured rows and the warnings raised while reading."""
    warnings: list[str] = []
    with csv_path.open(newline='', encoding='utf-8') as handle:
        reader = csv.DictReader(handle)
        if not reader.fieldnames:
            raise BenchmarkError(f'{csv_path} has no header row')
        if 'frame_ms' not in reader.fieldnames:
            raise BenchmarkError(f'{csv_path} has no frame_ms column')
        rows, skipped = [], 0
        for row in reader:
            raw = (row.get('frame_ms') or '').strip()
            try:
                frame_ms = float(raw)
            except ValueError:
                skipped += 1  # A truncated final row is common if a run is killed.
                continue
            if frame_ms <= 0 or not math.isfinite(frame_ms):
                skipped += 1
                continue
            row['frame_ms'] = frame_ms
            rows.append(row)
    if skipped:
        warnings.append(f'{skipped} row(s) had no usable frame_ms and were ignored')
    measured = [r for r in rows if (r.get('phase') or MEASURED_PHASE).strip() == MEASURED_PHASE]
    warmup = len(rows) - len(measured)
    if 'phase' not in (rows[0].keys() if rows else {}):
        warnings.append('no phase column; every row was treated as measured')
    return measured, warnings + ([f'{warmup} warm-up row(s) excluded'] if warmup else [])


def summarize(csv_path: Path) -> dict:
    measured, warnings = read_frames(csv_path)
    if not measured:
        raise BenchmarkError(f'{csv_path} contains no measured frames')

    frame_ms = [row['frame_ms'] for row in measured]
    ordered = sorted(frame_ms)
    elapsed = sum(frame_ms) / 1000.0
    engine_deltas = [v for v in (_optional_float(r, 'engine_delta_ms') for r in measured) if v is not None]
    gpu = [v for v in (_optional_float(r, 'gpu_ms') for r in measured) if v is not None]

    segments: dict[str, dict] = {}
    for row in measured:
        name = (row.get('segment_name') or 'unnamed').strip() or 'unnamed'
        bucket = segments.setdefault(name, {'sample_count': 0, 'elapsed_seconds': 0.0})
        bucket['sample_count'] += 1
        bucket['elapsed_seconds'] += row['frame_ms'] / 1000.0
    for bucket in segments.values():
        bucket['elapsed_seconds'] = round(bucket['elapsed_seconds'], 3)
        bucket['mean_fps'] = (round(bucket['sample_count'] / bucket['elapsed_seconds'], 2)
                              if bucket['elapsed_seconds'] > 0 else None)

    summary = {
        'sample_count': len(frame_ms),
        'elapsed_seconds': round(elapsed, 3),
        'mean_fps': round(len(frame_ms) / elapsed, 2) if elapsed > 0 else None,
        'median_ms': round(median(ordered), 3),
        'p95_ms': round(percentile(ordered, 95), 3),
        'p99_ms': round(percentile(ordered, 99), 3),
        'min_ms': round(ordered[0], 3),
        'max_ms': round(ordered[-1], 3),
        'frames_over_33_33ms': sum(1 for ms in frame_ms if ms > THRESHOLDS[0]),
        'frames_over_50ms': sum(1 for ms in frame_ms if ms > THRESHOLDS[1]),
        'frames_over_100ms': sum(1 for ms in frame_ms if ms > THRESHOLDS[2]),
        'engine_delta_ms_mean': round(sum(engine_deltas) / len(engine_deltas), 3) if engine_deltas else None,
        'gpu_ms_mean': round(sum(gpu) / len(gpu), 3) if gpu else None,
        'segments': segments,
        'warnings': warnings,
    }
    return summary


def load_metadata(folder: Path) -> dict:
    run_json = folder / 'run.json'
    if not run_json.is_file():
        return {'metadata_available': False}
    try:
        data = json.loads(run_json.read_text(encoding='utf-8'))
    except json.JSONDecodeError as error:
        return {'metadata_available': False, 'metadata_error': str(error)}
    keep = ('route_version', 'captured_utc', 'platform', 'mobile_preview', 'build_version',
            'engine_version', 'configuration', 'resolution', 'gpu_brand', 'device_make_model',
            'frame_cap', 'warmup_seconds', 'segment_scale', 'unavailable')
    metadata = {key: data.get(key) for key in keep}
    metadata['metadata_available'] = True
    return metadata


def resolve_csv(target: Path) -> Path:
    if target.is_dir():
        candidate = target / 'frames.csv'
        if not candidate.is_file():
            raise BenchmarkError(f'{target} has no frames.csv')
        return candidate
    if not target.is_file():
        raise BenchmarkError(f'{target} does not exist')
    return target


def format_report(summary: dict, metadata: dict) -> str:
    lines = ['County Line benchmark summary', '']
    if metadata.get('metadata_available'):
        resolution = metadata.get('resolution') or {}
        lines += [
            f"  route version   {metadata.get('route_version')}",
            f"  captured (UTC)  {metadata.get('captured_utc')}",
            f"  platform        {metadata.get('platform')}",
            f"  build           {metadata.get('build_version') or 'unavailable'}"
            f" / engine {metadata.get('engine_version') or 'unavailable'}"
            f" / {metadata.get('configuration') or 'unavailable'}",
            f"  resolution      {resolution.get('width', '?')}x{resolution.get('height', '?')}",
            f"  GPU             {metadata.get('gpu_brand') or 'unavailable'}",
            f"  device          {metadata.get('device_make_model') or 'unavailable'}",
            f"  frame cap       {metadata.get('frame_cap') or 'unavailable'}",
            f"  warm-up         {metadata.get('warmup_seconds')} s (excluded)",
        ]
        if metadata.get('mobile_preview'):
            lines.append('  NOTE            PC mobile preview; not a physical Android measurement')
    else:
        lines.append('  metadata        unavailable (no readable run.json)')
    lines += [
        '',
        f"  measured frames {summary['sample_count']}",
        f"  elapsed         {summary['elapsed_seconds']:.3f} s",
        f"  mean FPS        {summary['mean_fps']}  (frames / elapsed)",
        f"  median          {summary['median_ms']:.3f} ms",
        f"  p95             {summary['p95_ms']:.3f} ms",
        f"  p99             {summary['p99_ms']:.3f} ms",
        f"  min / max       {summary['min_ms']:.3f} / {summary['max_ms']:.3f} ms",
        f"  over 33.33 ms   {summary['frames_over_33_33ms']}",
        f"  over 50 ms      {summary['frames_over_50ms']}",
        f"  over 100 ms     {summary['frames_over_100ms']}",
        f"  engine delta    {summary['engine_delta_ms_mean'] if summary['engine_delta_ms_mean'] is not None else 'unavailable'} ms mean",
        f"  GPU time        {summary['gpu_ms_mean'] if summary['gpu_ms_mean'] is not None else 'unavailable'}",
    ]
    if summary['segments']:
        lines += ['', '  per segment:']
        for name, bucket in summary['segments'].items():
            lines.append(f"    {name:<18} {bucket['sample_count']:>6} frames  {bucket['mean_fps']} FPS mean")
    for warning in summary['warnings']:
        lines.append(f'  warning: {warning}')
    return '\n'.join(lines)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description='Summarize a County Line benchmark capture.')
    parser.add_argument('capture', type=Path, help='capture folder or frames.csv')
    parser.add_argument('--json', type=Path, help='also write the summary as JSON here')
    args = parser.parse_args(argv)
    try:
        csv_path = resolve_csv(args.capture)
        summary = summarize(csv_path)
    except BenchmarkError as error:
        print(f'benchmark summary failed: {error}', file=sys.stderr)
        return 2
    metadata = load_metadata(csv_path.parent)
    print(format_report(summary, metadata))
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps({'metadata': metadata, 'summary': summary}, indent=2), encoding='utf-8')
        print(f'\nwrote {args.json}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
