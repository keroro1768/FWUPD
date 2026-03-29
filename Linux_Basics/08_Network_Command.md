# 第8章：網路基礎 — Linux 的網路指令

> **本章目標**：學會用 Linux 指令檢查網路、測試連線、遠端登入  
> **前置知識**：會用基本指令（第二章）

---

## 🌐 先想一下 Windows 的網路工具

在 Windows，你可能用過：

| 工具 | 用途 | Linux 等同 |
|------|------|-----------|
| `ipconfig` | 查看 IP 設定 | `ip addr` |
| `ipconfig /release` | 釋放 DHCP | `dhclient -r` |
| `ping` | 測試網路連線 | `ping` |
| `tracert` | 追蹤路由 | `traceroute` |
| `nslookup` | 查 DNS | `nslookup` / `dig` |
| 網路芳鄰 | 區網探索 | `arp`, `nmblookup` |
| 遠端桌面 | 連線到其他電腦 | `ssh` |

---

## 📍 ip — 查看和設定網路

### ip 基本用法

```bash
# 查看所有網路介面的 IP 位址（最常用！）
ip addr

# 縮寫版
ip a

# 查看路由表（等同 ipconfig /routeprint）
ip route

# 查看 ARP 表格（等同 arp -a）
ip neigh
```

### 輸出範例

```bash
$ ip addr
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP
    link/ether 00:11:22:33:44:55 brd ff:ff:ff:ff:ff:ff
    inet 192.168.1.100/24 brd 192.168.1.255 scope global dynamic eth0
       valid_lft 86400sec preferred_lft 86400sec
```

**解讀**：

| 欄位 | 意義 |
|------|------|
| `lo` | Loopback 介面（永遠是 127.0.0.1） |
| `eth0` | 第一張乙太網路卡 |
| `inet 192.168.1.100/24` | IPv4 位址和子網路遮罩 |
| `link/ether 00:11:22:33:44:55` | MAC 位址 |
| `state UP` | 介面已啟動 |

### 啟動/關閉網路介面

```bash
# 關閉網路介面
sudo ip link set eth0 down

# 啟動網路介面
sudo ip link set eth0 up

# 設定 IP 位址
sudo ip addr add 192.168.1.50/24 dev eth0
```

---

## 📡 ping — 測試網路連線

### 基本用法

```bash
# 測試到 Google DNS 的連線
ping 8.8.8.8

# 測試到網址的連線
ping google.com

# Windows 預設只 ping 4 次，Linux 預設一直 ping
# Ctrl + C 停止
```

### 常用選項

```bash
ping -c 4 8.8.8.8        # 只 ping 4 次（等同 Windows 預設）
ping -i 2 8.8.8.8        # 每 2 秒發一個 request
ping -w 5 8.8.8.8        # 5 秒後自動停止
ping -s 1000 8.8.8.8     # 發送 1000 位元組的封包（預設 64）
```

### 輸出解讀

```
$ ping -c 4 8.8.8.8
PING 8.8.8.8 (8.8.8.8) 56(84) bytes of data.
64 bytes from 8.8.8.8: icmp_seq=1 ttl=117 time=10.2 ms
64 bytes from 8.8.8.8: icmp_seq=2 ttl=117 time=9.8 ms
64 bytes from 8.8.8.8: icmp_seq=3 ttl=117 time=9.5 ms
64 bytes from 8.8.8.8: icmp_seq=4 ttl=117 time=9.7 ms

--- 8.8.8.8 ping statistics ---
4 packets transmitted, 4 received, 0% packet loss, time 3005ms
rtt min/avg/max/mdev = 9.5/9.8/10.2/0.2 ms
```

| 欄位 | 意義 |
|------|------|
| `64 bytes` | 收到的封包大小 |
| `icmp_seq=1` | 第 1 個 ICMP 封包 |
| `ttl=117` | 剩餘壽命（經過 117 個路由器） |
| `time=10.2 ms` | 延遲時間 |
| `0% packet loss` | 沒有封包遺失 |

---

## 🔍 traceroute — 追蹤封包經過的路徑

### 基本用法

```bash
# 追蹤到目標的路徑
traceroute google.com

# Windows 等同：tracert google.com
```

### 輸出範例

```
traceroute to google.com (142.250.185.206), 30 hops max
 1  192.168.1.1     1.2 ms   1.1 ms   1.0 ms   ← 你的路由器
 2  10.0.0.1        5.3 ms   5.1 ms   5.2 ms   ← ISP
 3  72.14.215.85   10.5 ms  10.3 ms  10.4 ms   ← Google 網路
 4  142.250.185.206  9.8 ms   9.7 ms   9.8 ms   ← 目的地
```

> 如果某一 hop 顯示 `* * *`，代表那一個路由器沒有回應（可能是防火牆封鎖）。

---

## 🌐 curl 和 wget — 下載檔案

### curl

```bash
# 下載網頁
curl https://example.com

# 下載檔案並儲存
curl -O https://example.com/file.zip

# 下載並重新命名
curl -o newname.zip https://example.com/file.zip

# 只顯示 HTTP Header
curl -I https://example.com

# 跟蹤重新導向（-L）
curl -L https://example.com/redirect
```

### wget（更強大的下載工具）

```bash
# 下載檔案
wget https://example.com/file.zip

# 斷點續傳（網路中斷後可以繼續）
wget -c https://example.com/largefile.iso

# 後台下載
wget -b https://example.com/largefile.iso

# 觀看後台下載進度
jobs -l
```

---

## 🔐 ssh — 遠端登入

### 什麼是 SSH？

**SSH** = **S**ecure **Sh**ell

這是一個加密的遠端登入協議，讓你能安全地連線到其他 Linux 電腦。

> **類比**：SSH 等同於 Windows 的「遠端桌面連線」，但更輕量、純文字介面。

### 基本用法

```bash
# 連線到遠端伺服器
ssh username@server.example.com

# 指定連接埠（預設是 22）
ssh -p 2222 username@server.example.com

# 使用金鑰登入（不需要密碼）
ssh -i ~/.ssh/my_key.pem username@server.example.com
```

### SSH 金鑰登入

**密碼登入的問題**：密碼可能被側錄或暴力破解。

**SSH 金鑰**：產生一組公鑰/私鑰，用私鑰登入，完全不需要密碼。

```bash
# 1. 產生 SSH 金鑰
ssh-keygen -t ed25519 -C "your_email@example.com"

# 2. 產生的檔案：
#    ~/.ssh/id_ed25519      ← 私鑰（自己留著）
#    ~/.ssh/id_ed25519.pub ← 公鑰（放到伺服器）

# 3. 把公鑰複製到伺服器
ssh-copy-id username@server.example.com

# 4. 之後就可以直接登入，不需要密碼
ssh username@server.example.com
```

### SSH 常用選項

```bash
ssh -v username@server        # 詳細模式（除錯用）
ssh -X username@server        # 轉發 X11 圖形程式（Linux）
ssh -L 8080:localhost:80 user@server  # 連接埠轉發
```

---

## 📁 scp — 透過 SSH 複製檔案

### 基本用法

```bash
# 複製本地檔案到遠端伺服器
scp file.txt username@server:/home/username/

# 複製遠端伺服器檔案到本地
scp username@server:/path/to/file.txt ./

# 複製整個資料夾（-r）
scp -r myfolder/ username@server:/home/username/

# 指定連接埠（-P，大寫 P）
scp -P 2222 file.txt username@server:/home/username/

# 限制頻寬（-l，單位 Kbps）
scp -l 500 file.txt username@server:/home/username/
```

> **scp 語法**：`scp [選項] 來源 目的地`

---

## 🔥 防火牆 — ufw 和 firewall-cmd

### 什麼是防火牆？

**防火牆**是控制網路流量的「守門員」。它決定哪些連線可以進來或出去。

> **類比**：就像大樓的門禁系統，允許「受信任的人」進入，阻擋「可疑的人」。

### ufw（Ubuntu/Debian）

**ufw** = **U**complicated **F**ire**w**all（簡單防火牆）

```bash
# 查看防火牆狀態
sudo ufw status

# 啟用防火牆
sudo ufw enable

# 關閉防火牆
sudo ufw disable

# 允許 SSH 連線（重要！關防火牆前要先開）
sudo ufw allow ssh

# 允許特定連接埠
sudo ufw allow 80/tcp        # 允許 HTTP
sudo ufw allow 443/tcp       # 允許 HTTPS
sudo ufw allow 8080:8090/tcp # 允許範圍

# 阻擋特定連接埠
sudo ufw deny 23/tcp         # 阻擋 Telnet

# 刪除規則
sudo ufw delete allow 80/tcp

# 允許特定 IP
sudo ufw allow from 192.168.1.100

# 查看詳細狀態（含編號）
sudo ufw status numbered
```

### firewall-cmd（Fedora/RHEL）

**firewall-cmd** 是 Fedora/RHEL 系列的防火牆工具。

```bash
# 查看防火牆狀態
sudo firewall-cmd --state

# 查看已開啟的連接埠/服務
sudo firewall-cmd --list-all

# 開放 SSH（HTTP 等類似）
sudo firewall-cmd --add-service=ssh --permanent
sudo firewall-cmd --add-service=http --permanent
sudo firewall-cmd --add-port=8080/tcp --permanent

# 重新載入設定
sudo firewall-cmd --reload

# 移除規則
sudo firewall-cmd --remove-service=http --permanent

# 永久規則（--permanent）需要 reload 才生效
```

---

## 📊 Windows vs Linux 網路指令對照

| 功能 | Windows | Linux |
|------|---------|-------|
| 查看 IP | `ipconfig` | `ip addr` 或 `ifconfig`（舊）|
| 測試連線 | `ping` | `ping` |
| 追蹤路由 | `tracert` | `traceroute` |
| DNS 查詢 | `nslookup` | `nslookup` 或 `dig` |
| 查看連線 | `netstat` | `ss` 或 `netstat` |
| 下載檔案 | 瀏覽器下載 | `curl`, `wget` |
| 遠端登入 | 遠端桌面 / PuTTY | `ssh` |
| 檔案傳輸 | FTP / SMB | `scp`, `sftp` |
| 防火牆 | Windows Defender 防火牆 | `ufw`, `firewall-cmd`, `iptables` |

---

## 💡 實用網路除錯流程

### 情境：無法上網

```bash
# 1. 先看自己 IP 有沒有問題
ip addr

# 2. 測試區網（閘道器）
ping 192.168.1.1        # 通常是路由器 IP

# 3. 測試網際網路
ping 8.8.8.8

# 4. 測試 DNS
ping google.com

# 5. 如果 DNS 有問題，手動指定
echo "nameserver 8.8.8.8" | sudo tee /etc/resolv.conf
```

### 情境：SSH 連線失敗

```bash
# 1. 確認目標伺服器 SSH 有開
nc -zv server.example.com 22

# 2. 確認本機 ssh 用戶端正常
ssh -v username@server

# 3. 查看 SSH 服務是否在跑
sudo systemctl status sshd

# 4. 查看是否有別人佔用連接埠 22
sudo lsof -i :22
```

---

## 🔑 重點整理

1. **`ip addr`** 查看網路介面和 IP 位址（等同 `ipconfig`）
2. **`ping`** 測試網路連線是否正常
3. **`curl` / `wget`** 下載檔案或測試 HTTP
4. **`ssh`** 遠端登入 Linux 伺服器
5. **`scp`** 透過 SSH 傳輸檔案
6. **防火牆**：`ufw`（Ubuntu）或 `firewall-cmd`（Fedora）

---

## ▶️ 下一步

最後一章！[第9章：開發工具](./09_Development_Tools.md)，學習如何編譯程式和使用 Git！
