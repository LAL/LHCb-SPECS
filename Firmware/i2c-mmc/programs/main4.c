#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "i2c-dev.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define I2C_SLAVE_ADDR		0x20

#define	CMD_RED_LED_ON		0x00
#define	CMD_RED_LED_OFF		0x01
#define	CMD_GREEN_LED_ON	0x02
#define CMD_GREEN_LED_OFF	0x03
#define CMD_BLUE_LED_ON		0x04
#define CMD_BLUE_LED_OFF	0x05
	
#define CMD_FPGA_RESET		0x06
#define CMD_PCIE_PERST		0x07
#define CMD_RELOAD_FPGA		0x08
#define CMD_READ_INIT_DONE	0x09

#define CMD_READ_LOCAL_TEMP	0x10
#define CMD_READ_REMOTE1_TEMP	0x11
#define CMD_READ_REMOTE2_TEMP	0x12

#define CMD_READ_ALL_TEMP	0x13

#define CMD_SET_CON_JTAG_SOURCE	0x14
#define CMD_SET_AMC_JTAG_SOURCE	0x15
#define CMD_SET_MMC_JTAG_SOURCE	0x16

#define CMD_DCDC_DISABLE	0x17
#define CMD_DCDC_ENABLE		0x18

#define CMD_READ_VMON_STATUS0	0x19
#define CMD_READ_VMON_STATUS1	0x20
#define CMD_READ_VMON_STATUS2	0x21

#define CMD_READ_ADC_VMON1_LOW		0x22
#define CMD_READ_ADC_VMON1_HIGH 	0x23
#define CMD_READ_ADC_VMON2_LOW		0x24
#define CMD_READ_ADC_VMON2_HIGH 	0x25
#define CMD_READ_ADC_VMON3_LOW		0x26
#define CMD_READ_ADC_VMON3_HIGH 	0x27
#define CMD_READ_ADC_VMON4_LOW		0x28
#define CMD_READ_ADC_VMON4_HIGH 	0x29
#define CMD_READ_ADC_VMON5_LOW		0x30
#define CMD_READ_ADC_VMON5_HIGH 	0x31
#define CMD_READ_ADC_VMON6_LOW		0x32
#define CMD_READ_ADC_VMON6_HIGH 	0x33
#define CMD_READ_ADC_VMON7_LOW		0x34
#define CMD_READ_ADC_VMON7_HIGH 	0x35
#define CMD_READ_ADC_VMON8_LOW		0x36
#define CMD_READ_ADC_VMON8_HIGH 	0x37
#define CMD_READ_ADC_VMON9_LOW		0x38
#define CMD_READ_ADC_VMON9_HIGH 	0x39
#define CMD_READ_ADC_VMON10_LOW		0x40
#define CMD_READ_ADC_VMON10_HIGH 	0x41
#define CMD_READ_ADC_VCCA_LOW		0x42
#define CMD_READ_ADC_VCCA_HIGH		0x43
#define CMD_READ_ADC_VCCINP_LOW		0x44
#define CMD_READ_ADC_VCCINP_HIGH 	0x45
#define CMD_VMON_ADC			0x46

#define clearbuff()	while(getchar() != '\n')
#define pause()		printf("Appuyez sur ENTREE pour continuer...\n"); getchar()

#define MAX_BUFF_SIZE	0x20

char menu();

int main(int argc, char *argv[]){

	int file;
	char *filename = "/dev/i2c-0";

	char action;
	
	unsigned char buff[MAX_BUFF_SIZE];
	float calcul;
	int tmp;

	//On ouvre le bus i2c
	if ((file = open(filename, O_RDWR)) < 0) {
	    perror("Failed to open the i2c bus");
	    exit(1);
	}

	printf("Ouverture du bus i2c \t\t\t[OK]\n");

	//Configuration de l'adresse i2c de la CCPC
	if (ioctl(file,I2C_SLAVE,I2C_SLAVE_ADDR) < 0) {
		printf("Failed to acquire bus access and/or talk to slave.\n");
		exit(1);
	}

	printf("Connection a l'adresse 0x%x \t\t[OK]\n",I2C_SLAVE_ADDR);

	do{
		action = menu();

		switch(action){

			case CMD_RED_LED_ON:			i2c_smbus_write_byte_data(file,CMD_RED_LED_ON, 0x01); printf("Commande envoyee\n"); break;
			case CMD_RED_LED_OFF:			i2c_smbus_write_byte_data(file,CMD_RED_LED_OFF, 0x01); printf("Commande envoyee\n"); break;
			case CMD_GREEN_LED_ON:			i2c_smbus_write_byte_data(file,CMD_GREEN_LED_ON, 0x01); printf("Commande envoyee\n"); break;
			case CMD_GREEN_LED_OFF:			i2c_smbus_write_byte_data(file,CMD_GREEN_LED_OFF, 0x01); printf("Commande envoyee\n"); break;
			case CMD_BLUE_LED_ON:			i2c_smbus_write_byte_data(file,CMD_BLUE_LED_ON, 0x01); printf("Commande envoyee\n"); break;
			case CMD_BLUE_LED_OFF:			i2c_smbus_write_byte_data(file,CMD_BLUE_LED_OFF, 0x01); printf("Commande envoyee\n"); break;
			case CMD_FPGA_RESET:			i2c_smbus_write_byte_data(file,CMD_FPGA_RESET, 0x01); printf("Commande envoyee\n"); break;
			case CMD_PCIE_PERST:			i2c_smbus_write_byte_data(file,CMD_PCIE_PERST, 0x01); printf("Commande envoyee\n"); break;
			case CMD_RELOAD_FPGA:			i2c_smbus_write_byte_data(file,CMD_RELOAD_FPGA, 0x01); printf("Commande envoyee\n"); break;
			case CMD_SET_MMC_JTAG_SOURCE:		i2c_smbus_write_byte_data(file,CMD_SET_MMC_JTAG_SOURCE, 0x01); printf("Commande envoyee\n"); break;
			case CMD_SET_AMC_JTAG_SOURCE:		i2c_smbus_write_byte_data(file,CMD_SET_AMC_JTAG_SOURCE, 0x01); printf("Commande envoyee\n"); break;
			case CMD_SET_CON_JTAG_SOURCE:		i2c_smbus_write_byte_data(file,CMD_SET_CON_JTAG_SOURCE, 0x01); printf("Commande envoyee\n"); break;
			case CMD_DCDC_ENABLE:			i2c_smbus_write_byte_data(file,CMD_DCDC_ENABLE, 0x01); printf("Commande envoyee\n"); break;
			case CMD_DCDC_DISABLE:			i2c_smbus_write_byte_data(file,CMD_DCDC_DISABLE, 0x01); printf("Commande envoyee\n"); break;

			case CMD_READ_INIT_DONE:		printf("\n%s\n\n",i2c_smbus_read_byte_data(file,CMD_READ_INIT_DONE) == 1 ? "Init Done (OK)" : "Init Done (NOK)"); break;
			case CMD_READ_ALL_TEMP:			printf("\nTemperature locale : %d degres celsius \nTemperature remote 1 :%d degres celsius\nTemperature remote 2: %d degres celsius\n\n",i2c_smbus_read_byte_data(file,CMD_READ_LOCAL_TEMP),i2c_smbus_read_byte_data(file,CMD_READ_REMOTE1_TEMP),i2c_smbus_read_byte_data(file,CMD_READ_REMOTE2_TEMP)); break;
			case CMD_VMON_ADC:
				printf("\nMesures en cours .");
				fflush(stdout);
				
				i2c_smbus_write_byte_data(file,CMD_VMON_ADC, 0x0C);		//Initialisation

				for(buff[0] = 0; buff[0] < 0x0C; buff[0]++){					
					sleep(1);
					i2c_smbus_write_byte_data(file,CMD_VMON_ADC, buff[0]);
					printf(".");
					fflush(stdout);
				}
				printf("\n");
				
/*
				do{
					sleep(1);
					printf(".");
					fflush(stdout);
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VCCINP_LOW);
				}while((buff[0] & 0x01) == 0x00);
*/
				printf("\n");
		
				tmp = 0;
				do{
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON1_LOW);
					tmp++;
				}while(buff[0] == 0xFF && tmp < 3);
	
				tmp = 0;
				do{
					buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON1_HIGH);
					tmp++;
				}while(buff[1] == 0xFF && tmp < 3);

				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON1: Erreur de lecture\n");
				else if(buff[0] == 0xFE || buff[1] == 0xFE)
					printf("VMON1: Erreur de mesure\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON1: %0.2f Volt(s) \n",calcul*0.002); 
				}

				tmp = 0;
				do{
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON2_LOW);
					tmp++;
				}while(buff[0] == 0xFF && tmp < 3);

				tmp=0;
				do{
					buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON2_HIGH);				
					tmp++;
				}while(buff[1] == 0xFF && tmp < 3);

				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON2: Erreur de lecture\n");
				else if(buff[0] == 0xFE || buff[1] == 0xFE)
					printf("VMON2: Erreur de mesure\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON2: %0.2f Volt(s) \n",calcul*0.002); 
				}

				tmp=0;
				do{
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON3_LOW);				
					tmp++;
				}while(buff[0] == 0xFF && tmp < 3);
				
				tmp=0;
				do{
					buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON3_HIGH);				
					tmp++;
				}while(buff[1] == 0xFF && tmp < 3);
				
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON3: Erreur de lecture\n");
				else if(buff[0] == 0xFE || buff[1] == 0xFE)
					printf("VMON3: Erreur de mesure\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON3: %0.2f Volt(s) \n",calcul*0.002); 
				}

				tmp=0;
				do{
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON4_LOW);				
					tmp++;
				}while(buff[0] == 0xFF && tmp < 3);
				
				tmp=0;
				do{
					buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON4_HIGH);				
					tmp++;
				}while(buff[1] == 0xFF && tmp < 3);
				
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON4: Erreur de lecture\n");
				else if(buff[0] == 0xFE || buff[1] == 0xFE)
					printf("VMON4: Erreur de mesure\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON4: %0.2f Volt(s) \n",calcul*0.002); 
				}

				tmp=0;
				do{
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON5_LOW);				
					tmp++;
				}while(buff[0] == 0xFF && tmp < 3);
				
				tmp=0;
				do{
					buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON5_HIGH);				
					tmp++;
				}while(buff[1] == 0xFF && tmp < 3);
				
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON5: Erreur de lecture\n");
				else if(buff[0] == 0xFE || buff[1] == 0xFE)
					printf("VMON5: Erreur de mesure\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON5: %0.2f Volt(s) \n",calcul*0.002); 
				}

				tmp=0;
				do{
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON6_LOW);				
					tmp++;
				}while(buff[0] == 0xFF && tmp < 3);
				
				tmp=0;
				do{
					buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON6_HIGH);				
					tmp++;
				}while(buff[1] == 0xFF && tmp < 3);
				
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON6: Erreur de lecture\n");
				else if(buff[0] == 0xFE || buff[1] == 0xFE)
					printf("VMON6: Erreur de mesure\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON6: %0.2f Volt(s) \n",calcul*0.002); 
				}

				tmp=0;
				do{
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON7_LOW);				
					tmp++;
				}while(buff[0] == 0xFF && tmp < 3);
				
				tmp=0;
				do{
					buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON7_HIGH);				
					tmp++;
				}while(buff[1] == 0xFF && tmp < 3);
				
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON7: Erreur de lecture\n");
				else if(buff[0] == 0xFE || buff[1] == 0xFE)
					printf("VMON7: Erreur de mesure\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON7: %0.2f Volt(s) \n",calcul*0.002); 
				}

				tmp=0;
				do{
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON8_LOW);				
					tmp++;
				}while(buff[0] == 0xFF && tmp < 3);
				
				tmp=0;
				do{
					buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON8_HIGH);				
					tmp++;
				}while(buff[1] == 0xFF && tmp < 3);
				
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON8: Erreur de lecture\n");
				else if(buff[0] == 0xFE || buff[1] == 0xFE)
					printf("VMON8: Erreur de mesure\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON8: %0.2f Volt(s) \n",calcul*0.002); 
				}

				tmp=0;
				do{
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON9_LOW);				
					tmp++;
				}while(buff[0] == 0xFF && tmp < 3);
				
				tmp=0;
				do{
					buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON9_HIGH);				
					tmp++;
				}while(buff[1] == 0xFF && tmp < 3);
				
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON9: Erreur de lecture\n");
				else if(buff[0] == 0xFE || buff[1] == 0xFE)
					printf("VMON9: Erreur de mesure\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON9: %0.2f Volt(s) \n",calcul*0.002); 
				}

				tmp=0;
				do{
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON10_LOW);				
					tmp++;
				}while(buff[0] == 0xFF && tmp < 3);
				
				tmp=0;
				do{
					buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON10_HIGH);				
					tmp++;
				}while(buff[1] == 0xFF && tmp < 3);
				
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON10: Erreur de lecture\n");
				else if(buff[0] == 0xFE || buff[1] == 0xFE)
					printf("VMON10: Erreur de mesure\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON10: %0.2f Volt(s) \n",calcul*0.002); 
				}

				tmp=0;
				do{
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VCCA_LOW);				
					tmp++;
				}while(buff[0] == 0xFF && tmp < 3);
				
				tmp=0;
				do{
					buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VCCA_HIGH);				
					tmp++;
				}while(buff[1] == 0xFF && tmp < 3);
				
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VCCA: Erreur de lecture\n");
				else if(buff[0] == 0xFE || buff[1] == 0xFE)
					printf("VCCA: Erreur de mesure\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VCCA: %0.2f Volt(s) \n",calcul*0.002); 
				}

				tmp=0;
				do{
					buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VCCINP_LOW);				
					tmp++;
				}while(buff[0] == 0xFF && tmp < 3);
				
				tmp=0;
				do{
					buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VCCINP_HIGH);				
					tmp++;
				}while(buff[1] == 0xFF && tmp < 3);
				
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VCCINP: Erreur de lecture\n");
				else if(buff[0] == 0xFE || buff[1] == 0xFE)
					printf("VCCINP: Erreur de mesure\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VCCINP: %0.2f Volt(s) \n",calcul*0.002); 
				}


/*		
				buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON1_LOW);
				buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON1_HIGH);
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON1: Erreur de lecture\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON1: %0.2f Volt(s) \n",calcul*0.002); 
				}

				buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON2_LOW);
				buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON2_HIGH);
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON2: Erreur de lecture\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON2: %0.2f Volt(s)\n",calcul*0.002); 
				}

				buff[0] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON3_LOW);
				buff[1] = i2c_smbus_read_byte_data(file,CMD_READ_ADC_VMON3_HIGH);
				if(buff[0] == 0xFF || buff[1] == 0xFF)
					printf("VMON3: Erreur de lecture\n");
				else{
					calcul = (buff[1]*16)+(buff[0]>>4);
					printf("VMON3: %0.2f Volt(s)\n\n",calcul*0.002); 
				}
*/
				break;

			case -1: close(file);	return 0;
			default: close(file);	return -1;
		}

		pause();
	}while(1);

	
}

char menu(){
	
	int choix, choix_led, choix_etat, choix_fpga, choix_jtag, choix_alims, ret_scanf;

	do{
		do{
			system("clear");
			printf("Menu principal \n-------------------------\n\
				\n\t[0] Leds \
				\n\t[1] Temperatures \
				\n\t[2] Commandes FPGAS \
				\n\t[3] Source JTAG \
				\n\t[4] Alims \
				\n\t[5] Quitter \
				\n\n\t\tChoix : ");
			ret_scanf = scanf("%d",&choix);
			clearbuff();
		}while(ret_scanf != 1);

		if(choix == 0){

			do{
				do{
					system("clear");
					printf("Menu LEDs \n-------------------------\n\
						\n\t[0] Rouge \
						\n\t[1] Verte \
						\n\t[2] Bleue \
						\n\t[3] Retour \
						\n\n\t\tChoix : ");
					ret_scanf = scanf(" %d",&choix_led);
					clearbuff();
				}while(ret_scanf != 1);

				if(choix_led >= 0 && choix_led < 3){
					do{
						system("clear");
						printf("Menu LEDs \n-------------------------\n\
							\n\t[0] On \
							\n\t[1] Off \
							\n\t[2] Retour \
							\n\n\t\tChoix : ");
						ret_scanf = scanf(" %d",&choix_etat);
						clearbuff();
					}while((choix_etat != 0 && choix_etat != 1 && choix_etat != 2) || ret_scanf != 1);

					if(choix_etat == 0){
						switch(choix_led){
							case 0 : return CMD_RED_LED_ON;
							case 1 : return CMD_GREEN_LED_ON;
							case 2 : return CMD_BLUE_LED_ON;
						}
					}else if(choix_etat == 1){
						switch(choix_led){
							case 0 : return CMD_RED_LED_OFF;
							case 1 : return CMD_GREEN_LED_OFF;
							case 2 : return CMD_BLUE_LED_OFF;
						}
					}
				}
			}while(choix_led != 3);		
				
		}else if(choix == 1){
			return CMD_READ_ALL_TEMP;				
		}else if(choix == 2){
			do{
				do{
					system("clear");
					printf("Menu commandes FPGAs \n-------------------------\n\
						\n\t[0] Reset \
						\n\t[1] PCIe PERST Reset \
						\n\t[2] Reload FPGA \
						\n\t[3] Init Done \
						\n\t[4] Retour \
						\n\n\t\tChoix : ");
					ret_scanf = scanf("%d",&choix_fpga);
					clearbuff();
				}while(ret_scanf != 1);

				switch(choix_fpga){
					case 0: return CMD_FPGA_RESET;
					case 1: return CMD_PCIE_PERST;
					case 2: return CMD_RELOAD_FPGA;
					case 3: return CMD_READ_INIT_DONE;
				}
			}while(choix_fpga != 4);		
				
		}else if(choix == 3){
			do{
				do{
					system("clear");
					printf("Menu source JTAG \n-------------------------\n\
						\n\t[0] MMC \
						\n\t[1] Connecteur face avant \
						\n\t[2] Connecteur AMC \
						\n\t[3] Retour \
						\n\n\t\tChoix : ");
					ret_scanf = scanf("%d",&choix_jtag);
					clearbuff();
				}while(ret_scanf != 1);
				
				switch(choix_jtag){
					case 0: return CMD_SET_MMC_JTAG_SOURCE;
					case 1: return CMD_SET_CON_JTAG_SOURCE;
					case 2: return CMD_SET_AMC_JTAG_SOURCE;
				}
			}while(choix_jtag != 3);		
				
		}else if(choix == 4){
			do{
				do{
					system("clear");
					printf("Menu alims \n-------------------------\n\
						\n\t[0] Enable DCDC \
						\n\t[1] Disable DCDC \
						\n\t[2] Tensions sequenceur \
						\n\t[3] Retour \
						\n\n\t\tChoix : ");
					ret_scanf = scanf("%d",&choix_alims);
					clearbuff();
				}while(ret_scanf != 1);

				switch(choix_alims){
					case 0: return CMD_DCDC_ENABLE;
					case 1: return CMD_DCDC_DISABLE;
					case 2: return CMD_VMON_ADC;
				}
			}while(choix_alims != 3);
		}

		
	}while(choix != 5);

	return -1;
}
