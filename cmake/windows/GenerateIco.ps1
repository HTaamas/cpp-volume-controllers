param(
    [Parameter(Mandatory = $true)]
    [string]$InputPng,

    [Parameter(Mandatory = $true)]
    [string]$OutputIco
)

$inputPath = [System.IO.Path]::GetFullPath($InputPng)
$outputPath = [System.IO.Path]::GetFullPath($OutputIco)

if (-not (Test-Path -LiteralPath $inputPath)) {
    throw "Input PNG not found: $inputPath"
}

$pngBytes = [System.IO.File]::ReadAllBytes($inputPath)
$pngLength = [uint32]$pngBytes.Length

$outputDir = [System.IO.Path]::GetDirectoryName($outputPath)
if ($outputDir) {
    [System.IO.Directory]::CreateDirectory($outputDir) | Out-Null
}

$iconStream = [System.IO.File]::Open($outputPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
try {
    $writer = New-Object System.IO.BinaryWriter($iconStream)
    try {
        # ICONDIR
        $writer.Write([UInt16]0)
        $writer.Write([UInt16]1)
        $writer.Write([UInt16]1)

        # ICONDIRENTRY. Width/height of 0 means 256 px.
        $writer.Write([byte]0)
        $writer.Write([byte]0)
        $writer.Write([byte]0)
        $writer.Write([byte]0)
        $writer.Write([UInt16]1)
        $writer.Write([UInt16]32)
        $writer.Write([UInt32]$pngLength)
        $writer.Write([UInt32]22)

        # PNG image payload
        $writer.Write($pngBytes)
    }
    finally {
        $writer.Dispose()
    }
}
finally {
    $iconStream.Dispose()
}
