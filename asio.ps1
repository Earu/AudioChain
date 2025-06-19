$asioUrl = 'https://download.steinberg.net/sdk_downloads/asiosdk_2.3.3_2019-06-14.zip'
$userAgent = 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
Invoke-WebRequest -Uri $asioUrl -OutFile 'asio_sdk.zip' -UserAgent $userAgent -ErrorAction Stop
Expand-Archive -Path 'asio_sdk.zip' -DestinationPath 'external' -Force -ErrorAction Stop
Get-ChildItem -Path "external" -Directory -Filter "asiosdk*" | ForEach-Object {
    Rename-Item -Path $_.FullName -NewName "asio_sdk" -Force -ErrorAction Stop
}
