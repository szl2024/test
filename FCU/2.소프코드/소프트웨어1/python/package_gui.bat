@echo off
setlocal

set "ROOT=%~dp0.."
set "PY_DIR=%~dp0"

if not exist "%PY_DIR%cmd\fcu_cli" (
  echo python\cmd\fcu_cli not found. CLI source is required.
  exit /b 1
)

pushd "%ROOT%"
go build -o "%PY_DIR%fcu_cli.exe" ./python/cmd/fcu_cli
if errorlevel 1 (
  popd
  exit /b 1
)
popd

if not exist "%PY_DIR%fcu_cli.exe" (
  echo fcu_cli.exe not found. Build failed or missing Go source.
  exit /b 1
)

py -m PyInstaller --workpath "%PY_DIR%build" --distpath "%ROOT%" "%PY_DIR%FCU_Tool_GUI.spec"
