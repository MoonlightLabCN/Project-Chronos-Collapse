# Chronos Studio MVP

Chronos Studio is the developer-side packager for the Chronos Runtime.

A project contains `chronos.json`, one C source file, and one XAML fragment.
The packager cross-compiles the C source to an ARM Thumb-2 `.tccmod` module and
writes a single `.chronos` container. Its fixed layout is a small `CHRP` header
followed by a UTF-8 manifest, the native module, and the XAML fragment. A
normal user opens that package using the Chronos Runtime; the runtime validates
its layout, renders the XAML, resolves only its approved host API, and calls
the entry function.

## Developer UI

On a Windows development machine, launch the small graphical Studio with:

```powershell
pwsh -NoProfile -File .\ChronosStudio\Launch-ChronosStudio.ps1
```

It lets a developer select a local project folder, build it, and read the
generated `.chronos` path without using a terminal. The Studio and Runtime are
deliberately separate: developers distribute the small `.chronos` file, while
ordinary users install Chronos Runtime once.

## Build a project

```powershell
pwsh -NoProfile -File .\ChronosStudio\Build-ChronosProject.ps1 .\Samples\HelloChronos
```

The output is written to `Samples\HelloChronos\out\`.

## Build the user Runtime AppX

```powershell
pwsh -NoProfile -File .\ChronosStudio\Build-RuntimeDemo.ps1
```

This rebuilds the ARM UWP Runtime, embeds the sample package, and writes a
signed AppX under `ChronosArtifacts\RuntimeDemo\`. Before sideloading on a
phone, trust the matching `SandboxProbe.cer` certificate for the current user.

## Version 1 project descriptor

```json
{
  "format": "chronos-project",
  "formatVersion": 1,
  "id": "com.example.hello",
  "name": "Hello",
  "source": "main.c",
  "view": "view.xaml",
  "entry": "chronos_main"
}
```

The C source is intentionally compiled with `-nostdlib`. Runtime programs can
only call symbols the user-facing Runtime explicitly publishes.
