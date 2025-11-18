@ECHO OFF

REM --- CONFIGURATION ---
REM 1. Match the filename exactly to your upload
SET INPUT_FILE=DiffuseCM3.png

REM 2. We found the issue: Your image is a Horizontal Cross (4x3), not Vertical.
REM    We MUST use 'cube-from-hc' here.
SET ASSEMBLY_CMD=cube-from-hc

REM 3. Mipmap levels (0 = full chain)
SET MIP_LEVELS=9

REM 4. Output names
SET TEMP_FILE=Cubemap_TEMP.dds
SET FINAL_FILE=Cubemap.dds
REM ---------------------

ECHO.
ECHO --- Starting Unfolded Cubemap to DDS Converter ---
ECHO.

REM --- Step 1: Assemble the Horizontal Cross into a Cubemap ---
ECHO [1/2] Assembling horizontal cross layout...
REM Using 'cube-from-hc' because your image is wider than it is tall.
texassemble %ASSEMBLY_CMD% %INPUT_FILE% -o %TEMP_FILE% -y

IF ERRORLEVEL 1 (
    ECHO.
    ECHO ERROR in Step 1: Assembly failed.
    GOTO :END
)

REM --- Step 2: Resize, Generate Mipmaps, and Finalize ---
ECHO.
ECHO [2/2] Resizing to Power-of-2 and Generating Mipmaps...
texconv %TEMP_FILE% -o . --mip-levels %MIP_LEVELS% -pow2 -y

IF ERRORLEVEL 1 (
    ECHO.
    ECHO ERROR in Step 2: Mipmap generation failed.
    GOTO :END
)

REM --- Step 3: Cleanup and Rename ---
ECHO.
RENAME %TEMP_FILE% %FINAL_FILE%
ECHO.
ECHO Conversion Complete! "%FINAL_FILE%" is ready.

:END
pause