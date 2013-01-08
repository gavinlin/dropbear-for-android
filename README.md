drop dropbear folder into ANDROID_PATH/external and compile
$:. build/envsetup.sh
$:cd external/dropbear
$:mm

generate key on you devices:
$:mkdri /data/dropbear
$:cd /data/dropbear
$:dropbearkey -t rsa -f dropbear_rsa_host_key
$:dropbearkey -t dss -f dropbear_dss_host_key
$:dropbear -E
$:sftp-server
enjoy it
