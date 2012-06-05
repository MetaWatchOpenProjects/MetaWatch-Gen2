http://processors.wiki.ti.com/index.php/Portable_Projects

Build instruction:
1.Import the project:
- Select menu "Project -> Import Existing CCS Eclipse Project -> Select Search Directory:"
- Browse to the root folder of the project, e.g. "c:\MetaWatch-WDS11x", click "Finish" button.

2.Select a configuration
- There are 5 configurations for 5 different hardware: Analog Watch, Analog Devboard, Digital Watch, Digital Devboard, Digital BLE". 
- Right click the project name "Watch", choose "Build Configuration -> Set Active -> Digital or Digital BLE".

3. Build the configuration
- Select menu "Project -> Build Project".

4. Flash
- Select menu "Run -> Debug". The built image will be flashed to the watch.
- After flashing done, click "Resume" button on the toolbar to let it run. Click "Stop" button to stop debugging.

