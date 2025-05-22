# Linux Driver

## Supported opttions
- Board:
  - Geniatech: `artifactory.global.renesas.com/shared-conn-docker-registry-local/renesas/tin:latest`
  - Raspberri Py: `artifactory.global.renesas.com/shared-conn-docker-registry-local/renesas/rpi:v3.0.1`
  - Rzg3s: `artifactory.global.renesas.com/shared-conn-docker-registry-local/renesas/tin:rzg3s`
- Cips:
  - tin
  - vanadium
- Interface:
  - spi
  - sdio
- Revision:
  - aa
  - ba

For more details about support platforms run: `./linux_build.sh --help`

### Build one board, chip, intrface and revision
#### NOT Docker
```console
foo@bar:./linux_build.sh --action build --board geniatech --chip tin --interface spi --revision aa
```
#### Docker build
- 2 way to build:
  - Uncoment and update valus for variables in `.env` file
    ```.env
    IMAGE=artifactory.global.renesas.com/shared-conn-docker-registry-local/renesas/tin:latest
    BOARD=geniatech
    CHIP=tin
    INTERFACE=sdio
    REVISION=aa
    ```
    Run command
    ```console
    foo@bar: docker compose up build
    ```
  - Set value from console when run command
    ```console
    foo@bar:IMAGE=artifactory.global.renesas.com/shared-conn-docker-registry-local/renesas/tin:latest BOARD=geniatech CHIP=tin INTERFACE=sdio REVISION=aa docker compose up build
    ```
``NOTE: for debug buidl change servuice 'build' to 'debug'``
Result of build will store to folder: `build`

### Build multiple chips and intrafe for evrry board
Build will inlude all options of chips, interfases and revisions
- Regular build
  - Geniatech
      ```console
      foo@bar: docker compose up build_geniatech
      ```
  - Rzg3s:
      ```console
      foo@bar: docker compose up build_rpi
      ```
  - Rpi:
      ```console
      foo@bar: docker compose up build_rzg3s
      ```
- Debug build
  - Geniatech
      ```console
      foo@bar: docker compose up debug_build_geniatech
      ```
  - Rzg3s:
      ```console
      foo@bar: docker compose up debug_build_rpi
      ```
  - Rpi:
      ```console
      foo@bar: docker compose up debug_build_rzg3s
      ```

Result of build will store to folders: `(debug_)build_<board>_<chip>_<interface>_<revision>`

### Clean progect from build
```console
foo@bar: docker compose up clean
```