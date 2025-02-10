#include "system_ctrl.h"

using namespace maix;
using namespace maix::sys;

extern kvm_sys_state_t kvm_sys_state;
extern kvm_oled_state_t kvm_oled_state;

void gen_hostapd_conf(char* ap_ssid) {
  std::fstream hostapd_conf("/etc/hostapd.conf", std::ios_base::out|std::ios_base::trunc);
  hostapd_conf.exceptions(std::ios::failbit|std::ios::badbit);
  try {
    if(!hostapd_conf.is_open()) {
      // error handling
      throw std::runtime_error("Failed to open file");
    }
    hostapd_conf << "ctrl_interface" << "=" << "/var/run/hostapd" << std::endl;
    hostapd_conf << "ctrl_interface_group" << "=" << "0" << std::endl;
    hostapd_conf << "nsssid" << "=" << "NanoKVM" << std::endl;
    hostapd_conf << "hw_mode" << "=" << "g" << std::endl;
    hostapd_conf << "channel" << "=" << "1" << std::endl;
    hostapd_conf << "beacon_int" << "=" << "100" << std::endl;
    hostapd_conf << "dtim_period" << "=" << "2" << std::endl;
    hostapd_conf << "max_num_sta" << "=" << "255" << std::endl;
    hostapd_conf << "rts_threshold" << "=" << "-1" << std::endl;
    hostapd_conf << "macaddr_acl" << "=" << "0" << std::endl;
    hostapd_conf << "auth_algs" << "=" << "3" << std::endl;
    hostapd_conf << "wpa" << "=" << "2" << std::endl;
    hostapd_conf << "wpa_passphrase" << "=" << &ap_ssid << std::endl;
    hostapd_conf << "ieee80211n" << "=" << "1" << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << endl;
  }
  assert(std::filesystem::exists("/etc/hostapd.conf"))
}

void gen_udhcpd_conf() {
  std::fstream udhcpd_conf("/etc/udhcpd.wlan0.conf", std::ios_base::out|std::ios_base::trunc);
  udhcpd_conf.exceptions(std::ios::failbit|std::ios::badbit);
  try {
    if(!udhcpd_conf.is_open()) {
      // error handling
      throw std::runtime_error("Failed to open file");
    }
    udhcpd_conf << "start" << " " << "10.10.10.100" << std::endl;
    udhcpd_conf << "end" << " " << "10.10.10.200" << std::endl;
    udhcpd_conf << "interface" << " " << "wlan0" << std::endl;
    udhcpd_conf << "pidfile" << " " << "/var/run/udhcpd.wlan0.pid" << std::endl;
    udhcpd_conf << "lease_file" << " " << "/var/lib/misc/udhcpd.wlan0.leases" << std::endl;
    udhcpd_conf << "subnet" << " " << "255.255.255.0" << std::endl;
    udhcpd_conf << "option" << " " << "lease" << " " << "864000" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << endl;
  }
  assert(std::filesystem::exists("/etc/udhcpd.wlan0.conf"));
}

void gen_dnsmasq_conf() {
  std::fstream dnsmasq_conf("/etc/udhcpd.wlan0.conf", std::ios_base::out|std::ios_base::trunc);
  dnsmasq_conf.exceptions(std::ios::failbit|std::ios::badbit);
  try {
    if(!dnsmasq_conf.is_open()) {
      // error handling
      throw std::runtime_error("Failed to open file");
    }
    dnsmasq_conf << "bind-interfaces" << std::endl;
    dnsmasq_conf << "interface" << "=" << "wlan0" << std::endl;
    dnsmasq_conf << "dhcp-range" << "=" << "10.10.10.2,10.10.10.254" << std::endl;
    dnsmasq_conf << "address" << "=" << "/#/10.0.0.1" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << endl;
  }
  assert(std::filesystem::exists("/etc/udhcpd.wlan0.conf"));
}

uint8_t sta_connect_ap(void) {
  uint8_t RW_Data[10];  
  FILE *fp;
    fp = popen("hostapd_cli all sta | grep aid", "r");
    fgets((char*)RW_Data, 2, fp);
  pclose(fp);

  if(RW_Data[0] == 'a') return 1;
  else return 0;
}

uint8_t ssid_pass_ok(void) {
  if(access("/kvmapp/kvm/wifi_try_connect", F_OK) == 0) return 1;
  else return 0;
}

uint8_t wifi_connected(void) {
  uint8_t RW_Data[20];  
  FILE *fp;
    fp = popen("wpa_cli -i wlan0 status | grep \"wpa_state\"", "r");
    fgets((char*)RW_Data, 15, fp);
  pclose(fp);

  if(RW_Data[10] == 'S') return 0;
  else if(RW_Data[10] == 'C') return 1;
  else return 0;
}

void kvm_start_wifi_config_process(void) {
  if(kvm_sys_state.wifi_config_process == -1){
    // printf("[kvmw]开始配置wifi\n");
    kvm_sys_state.wifi_config_process = 0;
    kvm_sys_state.sub_page = 0;
  }
}

void kvm_wifi_web_config_process() {
  if(ssid_pass_ok()){
    system("rm /kvmapp/kvm/wifi_try_connect");
    system("/etc/init.d/S30wifi restart");
    kvm_sys_state.wifi_config_process = 3;
    // 尝试连接wifi
    if(kvm_sys_state.wifi_config_process == 3){
      // time::sleep_ms(WIFI_CONNECTION_DELAY);
      if(wifi_connected()){
        // 连上了
        kvm_sys_state.wifi_config_process = -1;
      } else {
        // 没连上
        kvm_sys_state.wifi_config_process = 0;
      }
    }
  }
}

void kvm_wifi_config_process() { 
  int temp;
  char ran_num;
  static char cmd[70];
  switch(kvm_sys_state.wifi_config_process){
    case -1:
      // printf("[kvms]Not in the process of configuring WiFi\n");
      break;
    case 0: // 启动wifi
      srand((unsigned)time::time_ms());
      for(temp = 0; temp < 4; temp++){
        do{
          ran_num = (char)rand()%10 + '0';
          if(temp == 0) break;
        } while (ran_num == kvm_sys_state.wifi_ap_pass[(temp-1)*2]);
        kvm_sys_state.wifi_ap_pass[temp*2]   = ran_num;
        kvm_sys_state.wifi_ap_pass[temp*2+1] = ran_num;
      }
      kvm_sys_state.wifi_ap_pass[8] = '\0';

      sprintf(cmd, "echo %s > /kvmapp/kvm/ap.pass", kvm_sys_state.wifi_ap_pass);
      system("echo NanoKVM > /kvmapp/kvm/ap.ssid");
      system(cmd);
      system("/etc/init.d/S30wifi ap");

      kvm_sys_state.wifi_config_process = 1;
      kvm_sys_state.sub_page = 1;
      break;
    case 1: // 等待设备连接
      if(sta_connect_ap()){
        kvm_sys_state.wifi_config_process = 2;
        kvm_sys_state.sub_page = 3;
      }
      break;
    case 2: // 等待输入帐号密码
      if(ssid_pass_ok()){
        system("rm /kvmapp/kvm/wifi_try_connect");
        system("/etc/init.d/S30wifi restart");
        kvm_sys_state.wifi_config_process = 3;
        kvm_sys_state.sub_page = 5;
      }
      break;
    case 3: // 尝试连接wifi
      time::sleep_ms(WIFI_CONNECTION_DELAY);
      if(wifi_connected()){
        // 连上了
        kvm_sys_state.wifi_config_process = -1;
        kvm_sys_state.page = 0;
        kvm_sys_state.sub_page = 0;
      } else {
        // 没连上
        kvm_sys_state.wifi_config_process = 0;
        kvm_sys_state.sub_page = 0;
      }
      break;
  }
}

uint8_t kvm_reset_password(void)
{
  FILE *fp = popen("bash", "w");
    if (fp == NULL) {
      perror("popen");
      return 0;
    }

  fputs("passwd root\n", fp);
  time::sleep_ms(10);
  fputs("root\n", fp);
  time::sleep_ms(10);
  fputs("root\n", fp);
  time::sleep_ms(10);
  fputs("rm /etc/kvm/pwd\n", fp);
  time::sleep_ms(10);
  fputs("sync\n", fp);
  time::sleep_ms(10);
  fputs("exit\n", fp); // 退出 bash

  if (pclose(fp) == -1) {
    perror("pclose");
    return 1;
  }
  return 0;
}
