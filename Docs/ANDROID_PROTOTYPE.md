# Android prototype: Galaxy Z Fold8

Status: Android project foundation in progress. Configuration and desktop tests do not establish Android compatibility; only a packaged APK installed on the phone can do that. This uses the current prototype graphics, not the generated concept screenshots.

## First device target

Samsung Galaxy Z Fold8, both unfolded and cover-screen landscape. ARM64, ASTC textures, mobile Vulkan with OpenGL ES fallback, no desktop Vulkan/SM5. Initial performance target is sustained 30 FPS. Render resolution and settings are conservative starting values, not measured Fold8 optimizations. Resizing is enabled; folding and unfolding still require device validation. Touch has two sticks plus interaction, Book, jog/walk toggle and pause. Bluetooth controller input shares the existing bindings.

## Build

Enable Android in Epic Games Launcher > Library > UE 5.8 > Options > Target Platforms. Install JDK 21 and the SDK/NDK versions listed in the installed engine's Engine/Config/Android/Android_SDK.json. The current local 5.8.2 installation requests API 36, build-tools 36.0.0 and NDK 27.2.12479018. These local requirements differ from portions of Epic's web documentation; follow the installed engine configuration.

Run Scripts/Build-Android.ps1 with -SdkRoot and -JavaRoot pointing to those installations. -CheckOnly checks prerequisites without claiming a build. The full script builds, cooks, and packages a Development APK with data included into Artifacts/Android, prints its path and SHA-256, and fails if no fresh APK exists. This is for direct device testing, not a Play Store release. Never commit signing keys or SDK installations.

The touch UI can be exercised on Windows with -CLMobilePreview -faketouches. This verifies UI only, not Android rendering or performance.

## Phone acceptance checklist

Install the APK, launch without the PC attached, walk/look/jog with simultaneous touches, interact with Pruitt and the desk, read the Book and save/relaunch. Repeat with a Bluetooth controller. Inspect text and touch targets on both screens; fold/unfold, rotate, interrupt with Home/lock and resume. Verify a 20-minute town walk and interior visit for heat, stable frame rate and memory. Check material/shader compatibility, sound, save persistence and update installation without deleting app data. Record actual Android version and device GPU before creating a model-specific profile.

## Cross-device progress for the final product

The PC and Android editions should use the same campaign data model. Their current local SaveGame containers already share the report and world-state structs, but automatic sync is NOT implemented. Do not copy files between live save slots or assume Google/Steam platform backup synchronizes across platforms.

Planned service-independent contract:
- One linked player account and stable campaign ID shared across platforms.
- A versioned portable campaign snapshot, including report/choices, discovered places and safe world location; explicit migrations and validation before loading.
- Local checkpoints continue offline. Graphics, touch layout and device settings remain local.
- Upload only completed saves using an expected server revision. Keep the previous valid snapshot for recovery.
- Download a newer compatible revision before play. If both devices advanced offline, preserve both versions and ask which to continue; never silently choose by device clock or merge story choices.
- Foreground/resume checks and retryable background upload; app termination must not lose the local save.
- Test account linking, offline conflict, interrupted upload, corrupt data, older app versions, sign-out and deletion.

Choose the account/storage provider after a working Android build and service requirements review. No backend, paid account, credentials or cloud upload has been created as part of this setup.

## Local setup and current validation (2026-09-24)

SDK tools are installed outside the repository at F:/The County Line/.android-toolchain/sdk, with JDK 21 under .android-toolchain/jdk. The packaging script discovers this sibling directory when explicit paths and environment variables are absent. Official SDK installation completed successfully with API 36, build-tools 36.0.0, NDK r27c and CMake 3.22.1. No global environment settings were changed.

Win64 editor compilation succeeded. All 20 existing Unreal automation suites, 10 map tests and the 134-check PC runtime smoke test passed after the mobile foundation was added. A later UI-only refinement adds scale-aware controls in a two-by-two upper-right cluster, leaving both joystick areas clear; its editor build also succeeded.

APK preflight currently stops at the missing UE Android target component (Engine/Binaries/Android/UnrealGame.target). The SDK file checks pass. The engine Turnkey VerifySdk command returned exit 0 but reported no available Android platform to verify; that exit code is NOT evidence of an Android-ready engine. No APK has been produced and no phone was listed by adb devices. There is no Android performance, rendering or physical-touch validation yet.

The final desktop touch preview ran at 1280x960 with -windowed -ForceRes -CLMobilePreview -faketouches and exited 0; its 134 runtime smoke checks passed. The INTERACT, BOOK, JOG/WALK and PAUSE labels are legible and the two virtual sticks are visible. Evidence: Verification/Android/DesktopTouchPreview-1280x960.png. The town-review camera is letterboxed; this is a Windows-rendered interface check, not an Android screenshot or a physical touch/multitouch test.
