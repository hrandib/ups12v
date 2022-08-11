import qbs
import qbs.FileInfo

Project {
    name: "UPS"
    references: [
        "chibios-qbs/chibios.qbs",
    ]

    property path CH_PATH: sourceDirectory + "/ChibiOS/"
    property string CORE: "cortex-m0"
    property string MCU: "STM32F070x6"

    Product {
        name: "config"

        Depends { name: "cpp" }

        Export {
            Depends { name: "cpp" }

            cpp.driverFlags: [
                "-mcpu=" + CORE,
                "--specs=nano.specs",
            ]

            cpp.commonCompilerFlags: [
                "-fdata-sections",
                "-ffunction-sections",
                "-flto=auto", "-ffat-lto-objects",
                "-mfloat-abi=soft"
            ]

            cpp.cxxFlags: [
                "-Wno-register",
                "-Wno-volatile",
                "-Wno-deprecated-volatile"
            ]

            cpp.cLanguageVersion: "gnu11"
            cpp.cxxLanguageVersion: "gnu++20"

            cpp.includePaths: [
                "config",
                "board",
                "utility",
                "usb"
            ]

            cpp.defines: [
                MCU,
                "CH_CUSTOMER_LIC_PORT_CM4=TRUE",
            ]

            cpp.linkerFlags: [
                "--gc-sections",
                "--defsym=__process_stack_size__=0x100",
                "--defsym=__main_stack_size__=0x200",
            ]

            cpp.positionIndependentCode: false
            cpp.enableExceptions: false
            cpp.enableRtti: false

            Group {
                name: "Config"
                prefix: "config/"
                files: [
                    "halconf.h",
                    "mcuconf.h",
                    "chconf.h",
                ]
            }

            Properties {
                condition: qbs.buildVariant === "release"
                cpp.debugInformation: false
                cpp.optimization: "small"
            }

            Properties {
                condition: qbs.buildVariant !== "release"
                cpp.debugInformation: true
                cpp.generateLinkerMapFile: true
                cpp.optimization: "none"
            }
        }
    }

    CppApplication {
        name: "ups-firmware"
        type: ["printsize"]

        Depends { name: "chibios" }
        Depends { name: "license" }
        Depends { name: "config" }

        consoleApplication: false
        cpp.executableSuffix: ".elf"

        cpp.includePaths: [
            project.CH_PATH + "/os/various/shell/"
        ]

        Group {
            name: "Compiled object file"
            fileTagsFilter: "application"
            qbs.install: true
        }

        Group {
            name: "Board"
            prefix: "board/"
            files: [
                "board.cpp",
                "board.h",
            ]
        }

        Group {
            name: "Utility"
            prefix: "utility/"
            files: [
                "*.h",
            ]
        }

        Group {
            name: "various"
            prefix: project.CH_PATH + "/os/various/"
            files: [
                "syscalls.c"
            ]
        }

        Group {
            name: "usb"
            prefix: "usb/"
            files: [
                "usbcfg.h",
                "usbcfg.c"
            ]
        }

        Group {
            name: "shell"
            prefix: project.CH_PATH + "/os/various/shell/"
            files: [
                "shell.h",
                "shell.c",
                "shell_cmd.h",
                "shell_cmd.c",
            ]
        }

        files: [
            "main.cpp",
        ]

        Group {
            name: "Linker files"
            prefix: cpp.libraryPaths + "/"
            fileTags: ["linkerscript"]
            files: [ project.MCU + ".ld"]
        }

        Rule {
                id: size
                inputs: ["application"]
                Artifact {
                    fileTags: ["printsize"]
                    filePath: "-"
                }
                prepare: {
                    function getBinutilsSize(arr) {
                        for (var i=0; i < arr.length; ++i) {
                            if ('binutilsPathPrefix' in arr[i]) {
                                return arr[i].binutilsPathPrefix + 'size';
                            }
                        }
                    }
                    var sizePath = getBinutilsSize(product.dependencies)
                    var args = [input.filePath]
                    var cmd = new Command(sizePath, args)
                    cmd.description = "File size: " + FileInfo.fileName(input.filePath)
                    cmd.highlight = "linker"
                    return cmd;
                }
            }

    } //CppApplication

} //Project
