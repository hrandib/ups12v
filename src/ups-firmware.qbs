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
                "-mfloat-abi=soft",
            ]

            cpp.cxxFlags: [
                "-Wno-register",
                "-Wno-volatile",
            ]

            cpp.cLanguageVersion: "gnu11"
            cpp.cxxLanguageVersion: "gnu++2b"

            cpp.includePaths: [
                "config",
                "board",
                "utility",
                "usb"
            ]

            cpp.defines: [
                MCU,
                "CH_CUSTOMER_LIC_PORT_CM0=TRUE",
            ]

            cpp.linkerFlags: [
                "--gc-sections",
                "--defsym=__process_stack_size__=0x100",
                "--defsym=__main_stack_size__=0x100",
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
                    "shellconf.h"
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
                //cpp.optimization: "none"
                cpp.commonCompilerFlags: [
                    "-O1"
                ]

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
        cpp.generateLinkerMapFile: false

        cpp.includePaths: [
            "impl",
            "drivers",
            "resources",
            "utility",
            project.CH_PATH + "/os/various/shell",
            project.CH_PATH + "/os/various/cpp_wrappers"
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
            name: "utility"
            prefix: "utility/"
            files: [
                "*.h",
                "*.cpp"
            ]
        }

        Group {
            name: "drivers"
            prefix: "drivers/"
            files: [
                "*.h",
                "*.cpp"
            ]
        }

        Group {
            name: "resources"
            prefix: "resources/"
            files: [
                "*.h",
                "*.cpp"
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
                "*.h",
                "*.cpp"
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

        Group {
            name: "cpp wrappers"
            prefix: project.CH_PATH + "/os/various/cpp_wrappers/"
            files: [
                "ch.hpp",
                "ch.cpp",
            ]
        }

        Group {
            name: "impl"
            prefix: "impl/"
            files: [
                "adc_handler.cpp",
                "adc_handler.h",
                "cal_data.h",
                "display_handler.cpp",
                "display_handler.h",
                "monitor.cpp",
                "monitor.h",
                "shell_handler.cpp",
                "shell_handler.h",
                "main.cpp",
            ]
        }

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
