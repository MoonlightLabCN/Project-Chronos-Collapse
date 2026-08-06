# Project Chronos Collapse - Lumia 950 XL Device Connection, File Push, & JIT Deployment Guide

This guide details the procedures for connecting to the Lumia 950 XL device, uploading test module files to it via the Windows Device Portal (WDP) API, and compiling/deploying UWP test applications with the correct capabilities.

---

## 1. Phase 1 Verification: JIT Code Execution Capabilities

In Phase 1, we successfully bypassed standard dynamic loading constraints on Windows 10 Mobile (ARM32, Lumia 950 XL) by verifying dynamic code generation.

### 1.1 Capability Configuration
Microsoft documentation incorrectly suggests that `codeGeneration` belongs to the restricted namespace `rescap`. On Windows 10 Mobile, you **must** configure it as a **standard capability** inside `Package.appxmanifest`:
```xml
<Capabilities>
  <Capability Name="internetClient" />
  <Capability Name="codeGeneration" />
</Capabilities>
```
If declared as `<rescap:Capability Name="codeGeneration" />` or omitted entirely, memory protection API changes will fail unconditionally with `ERROR_ACCESS_DENIED` (Code 5).

### 1.2 W^X Memory Enforcement Policy
Windows 10 Mobile enforces strict **Write XOR Execute (W^X)** isolation policies. Allocating pages as `PAGE_EXECUTE_READWRITE` directly fails with error code 87. Developers must follow this lifecycle sequence:
1. Allocate memory as read-write: `VirtualAllocFromApp(..., PAGE_READWRITE)`
2. Copy compiled Thumb-2 machine code into memory.
3. Flush instruction cache: `FlushInstructionCache(GetCurrentProcess(), address, size)`
4. Modify page protection to execute-read: `VirtualProtectFromApp(..., PAGE_EXECUTE_READ, ...)`
5. Run code. For ARM32, the target address **must** have its LSB set to `1` (Thumb-2 mode check).

---

## 2. Device Portal Connection & Setup

To deploy applications and upload files, configure the Windows Device Portal on the Lumia 950 XL:

### 2.1 Configuration Steps on Device
1. Navigate to: `Settings -> Update & Security -> For Developers`.
2. Enable **Developer Mode**.
3. Enable **Device Portal** and set up authentication credentials (or disable authentication if testing locally).
4. Note the device URL and IP address (e.g., `http://192.168.3.67:80` or `https://192.168.3.18:50443`).

### 2.2 Network & Firewalls
- Ensure the host development computer and Lumia 950 XL are on the same local subnet.
- If connections timeout, check if Windows Defender Firewall blocks outbound traffic on ports `80`, `443`, `8080`, or `50443`.

---

## 3. File Deployment & Appx Push Methods

Windows Device Portal requires specific application package names to write to an application's `LocalAppData` storage.

### 3.1 Method A: Bundle as App Assets (Recommended)
Add `.tccmod` files directly to the Visual Studio project as Content:
- Path in project: `Assets/test_module.tccmod`
- Build Action: `Content`
- Copy to Output Directory: `Copy always`

Inside UWP application code, retrieve the file via:
```cpp
auto installFolder = Windows::ApplicationModel::Package::Current->InstalledLocation;
```

### 3.2 Method B: Push via PowerShell / curl to LocalAppData
First, retrieve the package full name from the device list:
```bash
curl -s "http://192.168.3.67:80/api/appx/packagemanager/packages" | grep -i sandbox
```
Using the package name, upload the binary using WDP REST APIs:
```bash
PACKAGE_NAME="d4b1a80d-13f5-46ae-9524-fb617bf92d8f_1.0.0.0_arm__..."
curl -X PUT \
  -H "Content-Type: application/octet-stream" \
  --data-binary "@test_module.tccmod" \
  "http://192.168.3.67:80/api/filesystem/apps/file?packagefullname=${PACKAGE_NAME}&filename=test_module.tccmod"
```

### 3.3 Method C: Push to Public Documents
If target folder resolution is restricted, write to the shared Documents folder (requires `documentsLibrary` capability declared in the manifest):
```bash
curl -X PUT \
  -H "Content-Type: application/octet-stream" \
  --data-binary "@test_module.tccmod" \
  "http://192.168.3.67:80/api/filesystem/apps/file?knownfolderid=DocumentsLibrary&filename=Phase3/test_module.tccmod"
```

---

## 4. Useful WDP REST Commands

```bash
# Query operating system info
curl -k https://192.168.3.67/api/os/info

# Query list of running processes
curl -k https://192.168.3.67/api/resourcemanager/processes

# Query list of installed application packages
curl -k https://192.168.3.67/api/appx/packagemanager/packages

# Trigger a system restart
curl -k -X POST https://192.168.3.67/api/control/restart
```
