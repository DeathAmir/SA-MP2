# SAA2 Archive Builder

This folder contains a native Win32 GUI for creating SA-MP SAA2 archives.

## Build

Run the Build SAA2 Archive Tool GitHub Actions workflow. It compiles only the archive builder sources and publishes SAA2Builder.exe with its required key files as a downloadable artifact.

The application uses standard Windows file dialogs and controls. Select files, choose an output .saa path, and click Build SAA2.

Keep pkey.bin and skey.bin beside the executable. They are required by the original archive signing and encryption implementation.

The workflow does not build the SA-MP client, server, launcher, or other repository projects.
