# Recursive Rename Benchmark Analysis

This analysis uses `benchmark_recursive_rename.py` to compare recursive
collection rename performance for `file_naming_policy=consistent` and
`file_naming_policy=random`.

The benchmark creates one top-level collection, the requested number of direct
subcollections, and the requested number of total data objects distributed
across those subcollections. It times only the top-level `imv` operation.

Speedup is calculated as:

```text
consistent_seconds / random_seconds
```

## 3x3 Result Grid

| Subcollections | Data objects | consistent | random | Speedup |
| ---: | ---: | ---: | ---: | ---: |
| 10 | 10 | 0.1867s | 0.1809s | 1.03x |
| 10 | 100 | 0.3620s | 0.1669s | 2.17x |
| 10 | 1000 | 2.2083s | 0.2001s | 11.04x |
| 100 | 10 | 0.2174s | 0.1696s | 1.28x |
| 100 | 100 | 0.4100s | 0.1947s | 2.11x |
| 100 | 1000 | 2.2248s | 0.2098s | 10.60x |
| 1000 | 10 | 0.2563s | 0.2342s | 1.09x |
| 1000 | 100 | 0.4665s | 0.2470s | 1.89x |
| 1000 | 1000 | 2.4374s | 0.3223s | 7.56x |

## Marginal Comparisons

Holding subcollection count fixed, increasing data objects from 10 to 1000 had
the following effect.

| Subcollections | consistent increase | random increase | Speedup increase |
| ---: | ---: | ---: | ---: |
| 10 | 11.83x | 1.11x | 10.69x |
| 100 | 10.23x | 1.24x | 8.27x |
| 1000 | 9.51x | 1.38x | 6.91x |
| Average | 10.52x | 1.24x | 8.62x |

Holding data object count fixed, increasing subcollections from 10 to 1000 had
the following effect.

| Data objects | consistent increase | random increase | Speedup change |
| ---: | ---: | ---: | ---: |
| 10 | 1.37x | 1.29x | 1.06x |
| 100 | 1.29x | 1.48x | 0.87x |
| 1000 | 1.10x | 1.61x | 0.69x |
| Average | 1.26x | 1.46x | 0.87x |

## Linear Fit

A simple linear fit across the nine points estimates the following marginal
effects.

| Metric | Per 100 subcollections | Per 100 data objects |
| --- | ---: | ---: |
| consistent rename time | +0.0127s | +0.2089s |
| random rename time | +0.0086s | +0.0048s |
| Speedup | -0.126x | +0.862x |

For `consistent`, each additional 100 data objects had about 16.5x more impact
on recursive rename time than each additional 100 subcollections.

For `random`, the fitted per-object cost is much lower than for `consistent`.
The remaining random-policy cost is dominated more by collection traversal and
catalog bookkeeping than by physical rename operations for data objects.

The observed speedup therefore grows primarily with the number of data objects,
not with the number of subcollections. In this run, moving from 10 to 1000 data
objects increased speedup by 8.62x on average, while moving from 10 to 1000
subcollections changed speedup by only 0.87x on average.
