# Minimal OpenSSH client/server with patches for Android cell phones

OpenSSH is a complete implementation of the SSH protocol (version 2) for secure remote login, command execution and file transfer. It includes a client ``ssh`` and server ``sshd``, file transfer utilities ``scp`` and ``sftp`` as well as tools for key generation (``ssh-keygen``), run-time key storage (``ssh-agent``) and a number of supporting programs.

This is a port of OpenBSD's [OpenSSH](https://openssh.com) to most Unix-like operating systems, including Linux, OS X and Cygwin done by [https://github.com/openssh/openssh-portable](https://github.com/openssh/openssh-portable), and further tailored to limited Android environment (adb shell). Android shell does not provide user names and account information, such as /etc/passwd. Furthermore, this version limits encryption protocols to internal implementations. As a result, a complex dependency on `libcrypto` is lifted off, and `ed25519` becomes the only supported (and still - the best one!) encryption protocol (that is, no DSA or RSA support). The low-level installation process involves access to the base system and therefore requires a bootloader with permissive recovery mode, such as [TWRP](https://github.com/TeamWin/Team-Win-Recovery-Project).

## Documentation

The official documentation for OpenSSH are the man pages for each tool:

* [ssh(1)](https://man.openbsd.org/ssh.1)
* [sshd(8)](https://man.openbsd.org/sshd.8)
* [ssh-keygen(1)](https://man.openbsd.org/ssh-keygen.1)
* [ssh-agent(1)](https://man.openbsd.org/ssh-agent.1)
* [scp(1)](https://man.openbsd.org/scp.1)
* [sftp(1)](https://man.openbsd.org/sftp.1)
* [ssh-keyscan(8)](https://man.openbsd.org/ssh-keyscan.8)
* [sftp-server(8)](https://man.openbsd.org/sftp-server.8)

## Building Portable OpenSSH for Android

The instructions below assume armv7. You may need to adjust the compiler to target armv8 (arm64) variant.

* Obtain GCC ARM cross toolchain:

```
sudo apt install gcc-arm-linux-gnueabi
```

* Build Musl:

```
git clone git://git.musl-libc.org/musl
cd musl
mkdir build
cd build
../configure --prefix=$(pwd)/../install
```

* Build and install Bash 5.0, which is going to be used for the login shell as show here: https://github.com/dmikushin/bash-anrdoid

* Build OpenSSH with minimum dependencies, statically linking against Musl:

```
git clone https://github.com/dmikushin/openssh-android
cd openssh-android
mkdir build
cd build
LDFLAGS=-static CFLAGS="-D__ANDROID__ -O3 -fomit-frame-pointer -ffast-math" CC=$(pwd)/../../musl/install/bin/musl-gcc ../configure --prefix=$(pwd)/../install --host=arm-linux-gnueabi --without-zlib --without-openssl
```

## Installation

Reboot your phone into accessible recovery mode, such as TWRP. Mount /system partition as writable and use `adb` to roll the compiled OpenSSH binaries:

```
adb push ./ssh /system/bin/
adb push ./sshd /system/bin/
adb push ./ssh-keygen /system/bin/
adb push ./sshd_config /system/etc/ssh/
```

Furthermore, we would want to generate the host SSH keys by manually calling `ssh-keygen`:

```
adb shell
ssh-keygen -f /system/etc/ssh/ssh_host_ed25519_key -N '' -t ed25519
```

Generate another key pair to be used for remote authentication:

```
adb shell
ssh-keygen -f /system/etc/ssh/ssh_user_key -N '' -t ed25519
mv /system/etc/ssh/ssh_user_key.pub /system/etc/ssh/authorized_keys
exit
adb pull /system/etc/ssh/ssh_user_key ./
adb shell rm /system/etc/ssh/ssh_user_key
```

## Deployment

Reboot the phone into Android and launch the SSH server:

```
adb shell
/system/bin/sshd
exit
```

Given that the phone has a reachable IP address (e.g. the phone is connected to your home wireless router), connect to it directly:

```
$ ssh 192.168.12.202 -p 2222 -i ./ssh_user_key
shell@alto5_premium:/ $ uname -a
Linux localhost 3.10.72@LA.BR.1.2.9_rb1.29-MIUI-Kernel #1 SMP PREEMPT Tue Jan 9 15:35:54 MSK 2018 armv7l GNU/Linux
```

