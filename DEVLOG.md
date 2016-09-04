# This is not a CHANGELOG
It is a verbose running commentary with no relationship to versioning.
File should be read top-to-bottom for chronological order.
The contents of this file will be periodically archived and purged to keep the log related to the "current events" of the code base.

_---J. Ian Lindsay 2016.08.27_

------

### 2016.09.03

This update of SpatiumGestu was spurned by numerous platform API changes, and
  the need to test the threading abstraction against FreeRTOS, which this project
  was meant to facilitate.

    76932    3600    3976   84508   14a1c  Before removing the empty transport cases.
    76932    3600    3976   84508   14a1c  After. No change.
    76956    3600    3976   84532   14a34  BufferPipe no longer depends on chained constructor.
