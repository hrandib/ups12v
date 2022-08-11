const TextFile = require("qbs.TextFile")
const FileInfo = require("qbs.FileInfo")

function getDriverList(portPath, mcuFamily)
{
    //Correct not for all cases, "platform.mk" may differ
    const file = new TextFile(FileInfo.joinPaths(portPath, mcuFamily, "platform.mk"))
    fastForward(file)
    const result = []
    while(true) {
        var line = file.readLine()
        if(line.trim().length > 0) {
            result.push(getDriverDirectory(line))
        }
        else {
            break;
        }
    }
    file.close()
    return result
}

const fastForward = function(file) {
    while(file.readLine().indexOf("# Drivers compatible") < 0)
        ;
}

const getDriverDirectory = function(mkIncludeString) {
    const tokens = mkIncludeString.split("/")
    const driverNameIndex = tokens.indexOf("LLD") + 1
    return tokens[driverNameIndex]
}
