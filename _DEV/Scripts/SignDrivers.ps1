
# ************************************************************
#
#
# PostBuild-step to sign the drivers fscabi and GABI_ACPI
#
# Author: David, Wanner (external)
#
# Date:     2016-08-25 David, Wanner (external)         Intial (rewritten in  PowerShell -> gen_MSMs.bat, gen_product.bat)
#           2016-08-26 David, Wanner (external)         Update associated WorkItems
#           2016-08-29 David, Wanner (external)         Check build error
#           2016-09-07 David, Wanner (external)         Improve error behaviour
#           2016-12-13 David, Wanner (external)         Copy NuGet packages
#                      David, Wanner (external)         Refactoring
#                      David, Wanner (external)         Remove EasyPCProtection Setup
#           2017-03-16 David, Wanner (external)         DropLocation depends on source branch name
#           2017-03-27 David, Wanner (external)         Add Update-AssociatedWorkItems
#           2017-07-07 David, Wanner (external)         Refactoring
#           2017-07-09 Ahn, Ulrich                      Only one silent installation batch for 32 and 64 bit named inst_wpp_silent.bat
#           2017-07-09 David, Wanner (external)         New sign process
#			2017-13-10 Bretthauer, Birgit				removed unnecessary procedures for driver signing
#
#
# Function: Copy-Binaries
#           Update-AssociatedWorkItems
#
# Parameters: sUserName
#             sUserPassword
#
#
# ************************************************************


Write-Host "########################################"
Write-Host "#             Signing Drivers          #"
Write-Host "########################################"

# ******************#
#                   #
#                   #
#     Variables     #
#                   #
#                   #
# ******************#
$securePassword = $sUserPassword | ConvertTo-SecureString -AsPlainText -Force   
$credential = New-Object System.Management.Automation.PSCredential($sUserName, $securePassword)

# Build variables
[String] $CollectionURL = “$env:SYSTEM_TEAMFOUNDATIONCOLLECTIONURI“
[String] $BuildUrl = “$env:BUILD_BUILDURI“
[String] $project = “$env:SYSTEM_TEAMPROJECT“
[String] $BuildId = “$env:BUILD_BUILDID”
[String] $BuildNumber = “$env:BUILD_BUILDNUMBER”
[string] $sBinaryRoot = $env:BUILD_BINARIESDIRECTORY
[string] $sSourceRoot = $env:BUILD_SOURCESDIRECTORY
[string] $sBuildSourceBranch = $env:BUILD_SOURCEBRANCHNAME
[string] $sDropLoc = $env:DropLoc

# Environment variables
[string] $sInstallShieldPath = $env:_INSTALLSHIELD2016_PATH_
[string] $sBuildTools = $env:_BUILDTOOLS_
[string] $sCertificates = $env:_CERTIFICATES_
[string] $sCommonProgramFilesx86 = ${Env:CommonProgramFiles(x86)}


# Misc variables
[string] $LogfilePath = $sBinaryRoot + "\logs\signlog.txt"
[string] $sPythonPath = "C:\Python\App\python.exe"
[string] $sSignServer = "\\abg0636a.rdswlab.net\build\bin\signclient.py"




function Start-Signing{
        param(
            [string]
            $XMLConfigPath,

            [string]
            $Mode,

            [string]
            $File,

            [string]
            $Description
        )

        if ($XMLConfigPath -ne '') {
            [xml]$XmlDocument = Get-Content -Path $XMLConfigPath
            $aInclude = @()
            $aExclude = @()
            $XmlDocument.files.include.file | ForEach-Object { $aInclude += $_.name }
            $XmlDocument.files.exclude.file | ForEach-Object { $aExclude += $_.name }
        } else {
            $aInclude = @($File)
            $aExclude = @()   
        }

        function Send-FilesToBeSigned {
            param(
                [PSObject]
                $fileMapping
            )
            $fileMapping |
                ForEach-Object {
                    $sTargetPath = $_.Target

                    if (-not (Test-Path $_.Target)) {
                        New-Item -ItemType directory -Path $_.Target | Out-Null
                    }
                    Copy-Item -Path ($_.Source + '\' + $_.File) -Destination $_.Target -Force
                }
        }

        function Get-SignedFiles {
            param(
                [PSObject]
                $fileMapping
            )
            $fileMapping |
                ForEach-Object {
                    Copy-Item -Path ($_.Target + '\' + $_.File) -Destination $_.Source -Force
                }
        }

        function Remove-SignShare {
            Write-Host "Remove Sign Share"
            if (Test-Path -Path $sServerPath) {
                Remove-Item -Path $sServerPath -Recurse -Force
            } else {
                Write-Host 'Share could not be removed.'
            }
        }

        function Get-UnsignedFiles {
            param(
                [string]
                $BaseDir,

                [string[]]
                $Include,

                [string[]]
                $Exclude
            )
            $psoMapping = @()
            Get-ChildItem -Path $BaseDir -Include $Include -Exclude $Exclude -Recurse -File | 
                Foreach-Object {

                    $sSourcePath = $_.FullName
                    $sTargetPath = $sServerPath + $_.FullName.Replace($BaseDir,'')

                    $pso = New-Object PSObject
                    $pso | Add-Member NoteProperty -Name 'Source' -Value $_.FullName.Replace('\' + $_.BaseName + $_.Extension, '')
                    $pso | Add-Member NoteProperty -Name 'Target' -Value $sTargetPath.Replace('\' + $_.BaseName + $_.Extension, '')
                    $pso | Add-Member NoteProperty -Name 'File' -Value ($_.BaseName + $_.Extension)
                    $psoMapping += $pso
                }
            return $psoMapping
        }

        
        $sServerPath = ""

        $tcpConnection = New-Object System.Net.Sockets.TcpClient('SWSignServer.rdswlab.net', '9998')
        $tcpStream = $tcpConnection.GetStream()
        $reader = New-Object System.IO.StreamReader($tcpStream)
        $writer = New-Object System.IO.StreamWriter($tcpStream)
        $writer.AutoFlush = $true

        if ($tcpConnection.Connected) {

            $writer.WriteLine("get")
            $sServerPath = $reader.ReadLine()
            Write-Host "Sign Share: $sServerPath"
            $fileMapping = Get-UnsignedFiles -BaseDir $sBinaryRoot -Include $aInclude -Exclude $aExclude
            
            Send-FilesToBeSigned($fileMapping)

            if ($Description -ne '') {
                $writer.WriteLine("start $Mode description:$Description")
            } else {
                $writer.WriteLine("start $Mode")
            }

            $exitCode = $reader.ReadLine()
            Write-Host "Return value: $exitCode"

            Get-SignedFiles($fileMapping)
            
            if (-not (Test-Path "$sBinaryRoot\logs")) {
                Write-Host "Create directory $sBinaryRoot\logs"
                New-Item -ItemType directory -Path "$sBinaryRoot\logs" | Out-Null
            }
            Copy-Item -Path ($sServerPath + '\log.txt') -Destination ($sBinaryRoot + '\logs\signLog.txt') -Force

            Remove-SignShare
    
            switch ($exitCode) {
                0 { Write-Host 'Execution was successful.' }
                1 { 
                    Write-Warning 'Execution has failed.' 
                    Exit $exitCode
                }
                2 { 
                    Write-Warning 'Execution has completed with warnings.' 
                    Exit $exitCode
                }
            } 

        }

        $reader.Close()
        $writer.Close()
        $tcpConnection.Close()
    }

# ******************#
#                   #
#                   #
#        Main       #
#                   #
#                   #
# ******************#


    Write-Host "########################################"
    Write-Host "#      Sign drivers   				   #"
    Write-Host "########################################"

    Write-Host "Start-Signing -XMLConfigPath `"$sSourceRoot\Scripts\sign_all_cat.xml`" -Mode 'authenticode'"
    Start-Signing -XMLConfigPath "$sSourceRoot\Scripts\sign_all_cat.xml" -Mode 'authenticode'

    Write-Host "Start-Signing -XMLConfigPath `"$sSourceRoot\Scripts\sign_all_sys.xml`" -Mode 'authenticode'"
    Start-Signing -XMLConfigPath "$sSourceRoot\Scripts\sign_all_sys.xml" -Mode 'authenticode'
	



Write-Host "########################################"
Write-Host "#          End PostBuildStep           #"
Write-Host "########################################"