[CmdletBinding()]
param(
    [string]$ImagePath = "dist\mini-os.img",
    [string]$OutputPath = "dist\mini-os.iso"
)

$image = (Resolve-Path -LiteralPath $ImagePath).Path
$output = [System.IO.Path]::GetFullPath($OutputPath)
$outputDirectory = [System.IO.Path]::GetDirectoryName($output)

if (-not [System.IO.Directory]::Exists($outputDirectory)) {
    [System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
}

Add-Type -TypeDefinition @"
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;

public static class MiniOsIsoStreamWriter
{
    public static void Save(object source, string path)
    {
        IStream stream = (IStream)source;
        byte[] buffer = new byte[65536];
        System.IntPtr bytesRead = Marshal.AllocCoTaskMem(sizeof(int));

        try
        {
            using (FileStream output = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None))
            {
                while (true)
                {
                    Marshal.WriteInt32(bytesRead, 0);
                    stream.Read(buffer, buffer.Length, bytesRead);
                    int count = Marshal.ReadInt32(bytesRead);
                    if (count == 0)
                    {
                        break;
                    }

                    output.Write(buffer, 0, count);
                }
            }
        }
        finally
        {
            Marshal.FreeCoTaskMem(bytesRead);
        }
    }
}
"@

$filesystemImage = New-Object -ComObject IMAPI2FS.MsftFileSystemImage
$filesystemImage.FileSystemsToCreate = 3
$filesystemImage.VolumeName = "MINI_OS"

$imageStream = New-Object -ComObject ADODB.Stream
$imageStream.Type = 1
$imageStream.Open()
$imageStream.LoadFromFile($image)
$imageStream.Position = 0
$filesystemImage.Root.AddFile("mini-os.img", $imageStream)

$bootStream = New-Object -ComObject ADODB.Stream
$bootStream.Type = 1
$bootStream.Open()
$bootStream.LoadFromFile($image)
$bootStream.Position = 0

$bootOptions = New-Object -ComObject IMAPI2FS.BootOptions
$bootOptions.AssignBootImage($bootStream)
$bootOptions.Emulation = 2
$bootOptions.PlatformId = 0
$filesystemImage.BootImageOptions = $bootOptions

$result = $filesystemImage.CreateResultImage()
[MiniOsIsoStreamWriter]::Save($result.ImageStream, $output)

$imageStream.Close()
$bootStream.Close()
