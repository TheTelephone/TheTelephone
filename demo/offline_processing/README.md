Demo for offline processing using PureData and the readsfnow~ and writesfnow~.
This demo only copies the content while changing the two channels.

Note 1: The demo uses the loadbang feature to enabled DSP automatically.
Note 2: The demo quits itself after readsfnow~ reported EOF.

For offline processing run:
```bash
pd -batch demo_offline_processing.pd
```

For editing the patch run:
```bash
pd -noloadbang demo_offline_processing.pd
```