"""Tests for Scripts/Performance/summarize_benchmark.py.

These run without Unreal: they feed the parser known frame data and check the
reported statistics, warm-up exclusion, and how it behaves on empty, truncated
and partial captures.

Run: <engine>/Engine/Binaries/ThirdParty/Python3/Win64/python.exe Scripts/Tests/test_performance_summary.py
Standard library only.
"""
from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Performance'))

import summarize_benchmark as sb  # noqa: E402

HEADER = 'frame_index,monotonic_seconds,frame_ms,engine_delta_ms,gpu_ms,phase,segment_index,segment_name'


def write_capture(rows, header=HEADER, run_json=None):
    """Write a capture folder and return its path (cleaned up by the caller)."""
    folder = Path(tempfile.mkdtemp(prefix='cl_perf_'))
    lines = [header] + list(rows)
    (folder / 'frames.csv').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    if run_json is not None:
        (folder / 'run.json').write_text(json.dumps(run_json), encoding='utf-8')
    return folder


def frame_rows(values, phase='measured', segment='CourtStreet', engine=None, gpu=''):
    rows = []
    for index, ms in enumerate(values):
        delta = ms if engine is None else engine
        rows.append(f'{index},{index * 0.016:.6f},{ms},{delta},{gpu},{phase},0,{segment}')
    return rows


class PercentileTests(unittest.TestCase):
    def test_nearest_rank_matches_the_documented_index(self):
        data = [float(n) for n in range(1, 101)]  # 1..100 ms
        self.assertEqual(sb.percentile(data, 95), 95.0)
        self.assertEqual(sb.percentile(data, 99), 99.0)
        self.assertEqual(sb.percentile(data, 100), 100.0)
        self.assertEqual(sb.percentile(data, 0.5), 1.0)

    def test_percentile_of_a_single_sample_is_that_sample(self):
        self.assertEqual(sb.percentile([12.5], 95), 12.5)
        self.assertEqual(sb.percentile([12.5], 1), 12.5)

    def test_small_sets_round_up_to_a_real_sample(self):
        # ceil(0.95 * 4) = 4 -> the slowest of four frames.
        self.assertEqual(sb.percentile([10.0, 11.0, 12.0, 40.0], 95), 40.0)
        self.assertEqual(sb.percentile([10.0, 11.0, 12.0, 40.0], 50), 11.0)

    def test_median_handles_odd_and_even_counts(self):
        self.assertEqual(sb.median([1.0, 2.0, 3.0]), 2.0)
        self.assertEqual(sb.median([1.0, 2.0, 3.0, 5.0]), 2.5)

    def test_empty_input_is_an_error_not_a_zero(self):
        with self.assertRaises(sb.BenchmarkError):
            sb.percentile([], 95)
        with self.assertRaises(sb.BenchmarkError):
            sb.median([])


class SummaryTests(unittest.TestCase):
    def summarize(self, rows, **kwargs):
        folder = write_capture(rows, **kwargs)
        try:
            return sb.summarize(folder / 'frames.csv')
        finally:
            for child in folder.iterdir():
                child.unlink()
            folder.rmdir()

    def test_known_steady_data(self):
        """100 frames of exactly 20 ms is 50 FPS, with no slow frames."""
        summary = self.summarize(frame_rows([20.0] * 100))
        self.assertEqual(summary['sample_count'], 100)
        self.assertAlmostEqual(summary['elapsed_seconds'], 2.0, places=6)
        self.assertAlmostEqual(summary['mean_fps'], 50.0, places=6)
        self.assertAlmostEqual(summary['median_ms'], 20.0, places=6)
        self.assertAlmostEqual(summary['p95_ms'], 20.0, places=6)
        self.assertEqual(summary['frames_over_33_33ms'], 0)
        self.assertEqual(summary['frames_over_50ms'], 0)
        self.assertEqual(summary['frames_over_100ms'], 0)

    def test_uneven_frame_times(self):
        """Mean FPS follows total time, so slow frames cannot be averaged away."""
        values = [10.0] * 90 + [60.0] * 9 + [200.0]
        summary = self.summarize(frame_rows(values))
        self.assertEqual(summary['sample_count'], 100)
        self.assertAlmostEqual(summary['elapsed_seconds'], (900 + 540 + 200) / 1000.0, places=6)
        self.assertAlmostEqual(summary['mean_fps'], 100 / 1.64, places=2)
        self.assertAlmostEqual(summary['median_ms'], 10.0, places=6)
        self.assertAlmostEqual(summary['p95_ms'], 60.0, places=6)
        self.assertAlmostEqual(summary['p99_ms'], 60.0, places=6)
        self.assertAlmostEqual(summary['max_ms'], 200.0, places=6)
        self.assertEqual(summary['frames_over_33_33ms'], 10)
        self.assertEqual(summary['frames_over_50ms'], 10)
        self.assertEqual(summary['frames_over_100ms'], 1)

    def test_mean_fps_is_not_the_average_of_instantaneous_fps(self):
        """One 500 ms stall: 1000/mean-ms is far below the mean of per-frame FPS."""
        values = [10.0] * 99 + [500.0]
        summary = self.summarize(frame_rows(values))
        naive = sum(1000.0 / ms for ms in values) / len(values)
        self.assertAlmostEqual(summary['mean_fps'], 100 / 1.49, places=2)
        self.assertLess(summary['mean_fps'], naive - 20)

    def test_threshold_counts_are_strictly_greater(self):
        """A frame exactly on a limit is not counted as over it."""
        summary = self.summarize(frame_rows([33.33, 33.34, 50.0, 50.01, 100.0, 100.1]))
        self.assertEqual(summary['frames_over_33_33ms'], 5)
        self.assertEqual(summary['frames_over_50ms'], 3)
        self.assertEqual(summary['frames_over_100ms'], 1)

    def test_warmup_rows_are_excluded(self):
        """Warm-up frames change no statistic, but are reported as excluded."""
        rows = frame_rows([200.0] * 20, phase='warmup', segment='warmup') + frame_rows([20.0] * 50)
        summary = self.summarize(rows)
        self.assertEqual(summary['sample_count'], 50)
        self.assertAlmostEqual(summary['mean_fps'], 50.0, places=6)
        self.assertAlmostEqual(summary['max_ms'], 20.0, places=6)
        self.assertTrue(any('warm-up' in w for w in summary['warnings']))

    def test_capture_of_only_warmup_is_refused(self):
        with self.assertRaises(sb.BenchmarkError):
            self.summarize(frame_rows([20.0] * 5, phase='warmup'))

    def test_segments_are_reported_separately(self):
        rows = frame_rows([10.0] * 50, segment='Drugstore') + frame_rows([40.0] * 25, segment='NorthLane')
        summary = self.summarize(rows)
        self.assertEqual(summary['segments']['Drugstore']['sample_count'], 50)
        self.assertEqual(summary['segments']['NorthLane']['sample_count'], 25)
        self.assertAlmostEqual(summary['segments']['Drugstore']['mean_fps'], 100.0, places=1)
        self.assertAlmostEqual(summary['segments']['NorthLane']['mean_fps'], 25.0, places=1)

    def test_missing_optional_metrics_stay_unavailable(self):
        """Blank gpu_ms must read as unavailable, never as 0 ms."""
        summary = self.summarize(frame_rows([20.0] * 10, gpu=''))
        self.assertIsNone(summary['gpu_ms_mean'])
        self.assertIsNotNone(summary['engine_delta_ms_mean'])

    def test_present_optional_metrics_are_averaged(self):
        summary = self.summarize(frame_rows([20.0] * 10, gpu='7.5'))
        self.assertAlmostEqual(summary['gpu_ms_mean'], 7.5, places=3)

    def test_absent_optional_columns_are_tolerated(self):
        header = 'frame_index,frame_ms,phase,segment_name'
        rows = [f'{i},20.0,measured,CourtStreet' for i in range(10)]
        summary = self.summarize(rows, header=header)
        self.assertEqual(summary['sample_count'], 10)
        self.assertIsNone(summary['gpu_ms_mean'])
        self.assertIsNone(summary['engine_delta_ms_mean'])

    def test_rows_without_a_phase_column_count_as_measured(self):
        header = 'frame_index,frame_ms'
        summary = self.summarize([f'{i},25.0' for i in range(8)], header=header)
        self.assertEqual(summary['sample_count'], 8)
        self.assertAlmostEqual(summary['mean_fps'], 40.0, places=6)

    def test_truncated_final_row_is_ignored_with_a_warning(self):
        """A killed run leaves a half-written line; the rest still summarizes."""
        rows = frame_rows([20.0] * 10)
        rows.append('10,0.160,')
        summary = self.summarize(rows)
        self.assertEqual(summary['sample_count'], 10)
        self.assertTrue(any('usable frame_ms' in w for w in summary['warnings']))

    def test_nonsense_and_nonpositive_durations_are_ignored(self):
        rows = frame_rows([20.0] * 5)
        rows += ['5,0.1,0,16,,measured,0,CourtStreet',
                 '6,0.2,-3,16,,measured,0,CourtStreet',
                 '7,0.3,not-a-number,16,,measured,0,CourtStreet',
                 '8,0.4,nan,16,,measured,0,CourtStreet']
        summary = self.summarize(rows)
        self.assertEqual(summary['sample_count'], 5)

    def test_empty_and_headerless_captures_fail_clearly(self):
        for rows, header in (([], HEADER), ([], 'frame_index,phase')):
            folder = write_capture(rows, header=header)
            try:
                with self.assertRaises(sb.BenchmarkError):
                    sb.summarize(folder / 'frames.csv')
            finally:
                (folder / 'frames.csv').unlink()
                folder.rmdir()

    def test_a_completely_empty_file_fails_clearly(self):
        folder = Path(tempfile.mkdtemp(prefix='cl_perf_'))
        try:
            (folder / 'frames.csv').write_text('', encoding='utf-8')
            with self.assertRaises(sb.BenchmarkError):
                sb.summarize(folder / 'frames.csv')
        finally:
            (folder / 'frames.csv').unlink()
            folder.rmdir()


class MetadataAndCliTests(unittest.TestCase):
    def test_metadata_is_read_and_missing_metadata_is_flagged(self):
        run = {'route_version': 1, 'platform': 'Windows', 'resolution': {'width': 1920, 'height': 1080},
               'gpu_brand': 'Example GPU', 'device_make_model': None, 'frame_cap': 'uncapped',
               'build_version': None, 'engine_version': '5.8.3', 'configuration': 'Development',
               'warmup_seconds': 5.0, 'mobile_preview': False, 'unavailable': ['gpu_ms']}
        folder = write_capture(frame_rows([20.0] * 10), run_json=run)
        try:
            metadata = sb.load_metadata(folder)
            self.assertTrue(metadata['metadata_available'])
            self.assertEqual(metadata['route_version'], 1)
            self.assertIsNone(metadata['device_make_model'])
            report = sb.format_report(sb.summarize(folder / 'frames.csv'), metadata)
            self.assertIn('unavailable', report)       # device and build are unknown here
            self.assertIn('1920x1080', report)
            self.assertNotIn('mobile preview', report)
        finally:
            for child in folder.iterdir():
                child.unlink()
            folder.rmdir()

    def test_mobile_preview_is_labelled_in_the_report(self):
        run = {'route_version': 1, 'platform': 'Windows (mobile preview on PC, not a physical Android measurement)',
               'mobile_preview': True, 'resolution': {'width': 1280, 'height': 720}}
        folder = write_capture(frame_rows([20.0] * 5), run_json=run)
        try:
            report = sb.format_report(sb.summarize(folder / 'frames.csv'), sb.load_metadata(folder))
            self.assertIn('not a physical Android measurement', report)
        finally:
            for child in folder.iterdir():
                child.unlink()
            folder.rmdir()

    def test_unreadable_metadata_does_not_stop_the_summary(self):
        folder = write_capture(frame_rows([20.0] * 5))
        try:
            (folder / 'run.json').write_text('{ truncated', encoding='utf-8')
            metadata = sb.load_metadata(folder)
            self.assertFalse(metadata['metadata_available'])
            self.assertIn('metadata_error', metadata)
            report = sb.format_report(sb.summarize(folder / 'frames.csv'), metadata)
            self.assertIn('metadata        unavailable', report)
        finally:
            for child in folder.iterdir():
                child.unlink()
            folder.rmdir()

    def test_cli_reports_and_writes_json(self):
        folder = write_capture(frame_rows([20.0] * 10))
        try:
            out = folder / 'summary.json'
            self.assertEqual(sb.main([str(folder), '--json', str(out)]), 0)
            written = json.loads(out.read_text(encoding='utf-8'))
            self.assertEqual(written['summary']['sample_count'], 10)
        finally:
            for child in folder.iterdir():
                child.unlink()
            folder.rmdir()

    def test_cli_fails_on_a_missing_capture(self):
        self.assertEqual(sb.main([str(Path(tempfile.gettempdir()) / 'cl_perf_missing_capture')]), 2)


if __name__ == '__main__':
    suite = unittest.TestSuite([
        unittest.defaultTestLoader.loadTestsFromTestCase(PercentileTests),
        unittest.defaultTestLoader.loadTestsFromTestCase(SummaryTests),
        unittest.defaultTestLoader.loadTestsFromTestCase(MetadataAndCliTests),
    ])
    # Report on stdout: PowerShell turns native stderr into an error record.
    outcome = unittest.TextTestRunner(stream=sys.stdout, verbosity=2).run(suite)
    sys.exit(0 if outcome.wasSuccessful() else 1)
