#include "sgl.h" // Required for basic sgl functions
#include "sgl_cd.h" // Required for cd access
#include "sega_bup.h" // Required for backup memory functions
#include "sega_per.h" // Required for controller input
#include "sega_gfs.h" // Requred for GFS directory functions

#define BUP_START 0x6070000 // This is where the backup library will expand to
#define MAX_FILE 50 // Needed for CD functions
#define MAX_DIR 10 // Needed for CD Functions
#define READSECT 50 // Needed for CD functions
#define  PAD_NUM 13 // Required for controller input

Uint32 workmemory[2048]; // Needed for BackUpFunctions
Sint32 dirwork[SLCD_WORK_SIZE(MAX_FILE)/sizeof(Sint32)]; // Needed for CD function
Uint8 readbuf[(READSECT*CDBUF_FORM1/sizeof(Uint8))]; // Needed for CD function

// General Functions
void clear(int y); // Clears the screen from y down
void clearInput(); // Clears input from the controller
void getBlockSize(Sint32 bytes, Sint32* blocks); // Converts bytes to blocksize
void pressStart(); // Displays Press Start on the screen, waits for controller input. Simliar to system("pause");
void renameFile(Sint32 fid, char* fnamePointer); // Reads a 8.3 filename from the cd and returns it as 11 characters

// Backup Functions, 0 = internal memory, 1 = external memory, 2 = floppy drive
void backupToBackup(int sourceDevice, int targetDevice); // Copies save from one backup device to another
void cdToBackup(int device); // Copies save from CD to backup device
void deleteSave(int device); // Deletes a save from backup device
void formatDevice(int device); // Formats backup device

// Controller assignments
static Uint16 pad_asign[]=
{
	PER_DGT_KU,
	PER_DGT_KD,
	PER_DGT_KR,
	PER_DGT_KL,
	PER_DGT_TA,
	PER_DGT_TB,
	PER_DGT_TC,
	PER_DGT_ST,
	PER_DGT_TX,
	PER_DGT_TY,
	PER_DGT_TZ,
	PER_DGT_TR,
	PER_DGT_TL,
};

void ss_main(void)
{
	// Variables needed for Backup functions
	BupConfig conf[3]; // stores backup configuration
	BupStat sttb; // stores backup status
	BupDir writetb; // writeb holds file data
	BupStat checkConnect;
	Bool externalMem = TRUE;
	Bool floppyMem = TRUE;

	// Variables needed for main menu
	Sint32 cursor=0; // Initial value of the cursor
	Uint16 data;	 // Used for controller input
	Uint16 mainMenuCounter = 0; // Used for setting up my main menu
	Uint16 mainMenu[5]; // Used for setting up my main menu;

	// Initializing functions
	slInitSystem(TV_320x224, NULL, 1); // Initializes screen
	BUP_Init(BUP_START, workmemory, conf); // Initializes back up cart

	// Checks if external backup device is connected
	if(BUP_Stat(1, 0, &checkConnect)==BUP_NON)
	{
				externalMem = FALSE;
	}

	// Checks if floppy backup device is connected
	if(BUP_Stat(2, 0, &checkConnect)==BUP_NON)
	{
				floppyMem = FALSE;
	}

   	// This while loop encompasses the menu as well as the switch.
	// It breaks when the cursor is on Quit and the user hits A,C, or start.
    do
    {
			mainMenuCounter = 0; // Needs to be rezeroed every loop

			// Title
		  	slPrint("Save Game Copier Ver 2.00", slLocate(2,1));
		  	slPrint("-------------------------", slLocate(2,2));

			// Main Menu options
			slPrint("Internal Memory", slLocate(5, mainMenuCounter + 4));
				mainMenu[mainMenuCounter] = 0;
				mainMenuCounter++;


			if(externalMem)
			{
				slPrint("External Memory", slLocate(5,mainMenuCounter + 4));
					mainMenu[mainMenuCounter] = 1;
					mainMenuCounter++;
			}

			if(floppyMem)
			{
				slPrint("Floppy Memory", slLocate(5,mainMenuCounter + 4));
					mainMenu[mainMenuCounter] = 2;
					mainMenuCounter++;
			}

			slPrint("CD Memory", slLocate(5,mainMenuCounter + 4));
				mainMenu[mainMenuCounter] = 3;
				mainMenuCounter++;

		  	slPrint("Credits", slLocate(5,mainMenuCounter + 4));
		  		mainMenu[mainMenuCounter] = 4;

			// This loop will display the menu, allow the user to move up and down in the menu choices,
	    	// and will exit when the user hits A,C, or Start.
	        // Then a switch statement will be executed, using the value of cursor as the key.
		    do
			{

	 			// Erases the old cursor
		  		slPrint("   ", slLocate(2,cursor+4));

				// Checks the controller for input
				data = Smpc_Peripheral[0].data;

		     	// Checks if up or down has been pressed, and if it has,
	        	// moves the cursor accordingly. Also wraps the top and bottom,
		        // meaning if you go all the way to the bottom and hit down,
        		// the cursor should appear at the top and vice versa.
		     	if((data & pad_asign[1])== 0)
		     	{
			  		cursor = (cursor+1)%(mainMenuCounter+1);
				}

	  	        if((data & pad_asign[0])== 0)
				{
					cursor = (cursor-1)%(mainMenuCounter+1);

	        		if(cursor<0)
	        	    {
			    		cursor = mainMenuCounter;
				    }
				}

				// Writes the cursor to the screen
  				slPrint(">>", slLocate(2,cursor+4));

				// Why the slSynch()s? I'm not really sure...
				// I find it slows down the program to a usable speed.
				// There's probably a better method.
				slSynch();
				slSynch();
				slSynch();

			}while((data & pad_asign[7])!= 0 && (data & pad_asign[6])!= 0 && (data & pad_asign[5])!= 0 &&  (data & pad_asign[4])!= 0); // End of do-while loop

			// Check if user pressed 'B'
			if((data & pad_asign[5])== 0)
			{
				mainMenu[cursor] = 4;
			}

			clearInput();

			switch(mainMenu[cursor]){

				case 0: // Internal Memory Menu
				{
					do{
						Sint32 subCursor = 0;
						Uint16 subMenu = 4;
						Uint16 subOffset = cursor + 5;
						Uint16 subMenuCounter = 0;
						Uint16 subMainMenu[5]; // Used for setting up my main menu;

						clear(3);

						slPrint("Internal Memory", slLocate(5, subMenu++));

						// Sub Menu options
						slPrint("Format", slLocate(8, subMenu++));
							subMainMenu[subMenuCounter]=0;
							subMenuCounter++;


						slPrint("Delete Save(s)", slLocate(8, subMenu++));
							subMainMenu[subMenuCounter]=1;
							subMenuCounter++;

						if(externalMem)
						{
							slPrint("Copy Save to External Memory", slLocate(8, subMenu++));
								subMainMenu[subMenuCounter]=2;
								subMenuCounter++;
						}

						if(floppyMem)
						{
							slPrint("Copy Save to Floppy Memory", slLocate(8, subMenu++));
							subMainMenu[subMenuCounter]=3;
							subMenuCounter++;
						}

						subMenuCounter--; // I need to subtract once for some reason

						// Rest of the main menu options
						if(externalMem)
						{
							slPrint("External Memory", slLocate(5,subMenu++));
						}

						if(floppyMem)
						{
							slPrint("Floppy Memory", slLocate(5,subMenu++));
						}
						slPrint("CD Memory", slLocate(5,subMenu++));
						slPrint("Credits", slLocate(5,subMenu++));


						// This loop will display the menu, allow the user to move up and down in the menu choices,
			    		// and will exit when the user hits A,C, or Start.
			        	// Then a switch statement will be executed, using the value of cursor as the key.
						do
						{

							// Erases the old cursor
					  		slPrint("  ", slLocate(5, subCursor + subOffset));

							// Checks the controller for input
							data = Smpc_Peripheral[0].data;

					     	// Checks if up or down has been pressed, and if it has,
				        	// moves the cursor accordingly. Also wraps the top and bottom,
					        // meaning if you go all the way to the bottom and hit down,
						    // the cursor should appear at the top and vice versa.
							if((data & pad_asign[1])== 0)
							{
								subCursor = (subCursor+1)%(subMenuCounter+1);
							}

							if((data & pad_asign[0])== 0)
							{
								subCursor = (subCursor-1)%(subMenuCounter+1);

							   	if(subCursor<0)
							    {
									subCursor = (subMenuCounter);
							    }
							}

							// Writes the cursor to the screen
						  	slPrint(">>", slLocate(5,subCursor + subOffset));

							// Why the slSynch()s? I'm not really sure...
							// I find it slows down the program to a usable speed.
							// There's probably a better method.
							slSynch();
							slSynch();
							slSynch();

						}while((data & pad_asign[7])!= 0 && (data & pad_asign[6])!= 0 && (data & pad_asign[5])!= 0 && (data & pad_asign[4])!= 0); // End of do-while loop

						// Check if user pressed 'B'
						if((data & pad_asign[5])== 0)
						{
								subMainMenu[subCursor] = 4;
						}

						clearInput();

						switch(subMainMenu[subCursor]){

										case 0: // Format Memory
										{
											formatDevice(0);
											clear(3);
											break;
										} // Format Memory

										case 1: // Delete Save(s)
										{

											deleteSave(0);
											clear(3);
											break;

										} // Delete Save(s)


										case 2: // Copy to External Memory
										{
											backupToBackup(0,1);
											clear(3);
											break;

										} // External Memory


										case 3: // Copy to Floppy Memory
										{
											backupToBackup(0,2);
											clear(3);
											break;

										} // Floppy Memory

										case 4: // user hit 'B'
										{
											// do nothing
											break;

										} // user hit 'B'


										default:
											// Should never come here
											slPrint("That's not possible", slLocate(2,11));

			 			}// end of switch statement


					}while((data & pad_asign[5])!= 0); // End of do-while loop

						//pressStart();
						clear(3);
						break;

				} // Internal Memory

				case 1: // External Memory Menu
				{
						do{
								Sint32 subCursor = 0;
								Uint16 subMenu = 4;
								Uint16 subOffset = cursor + 5;
								Uint16 subMenuCounter = 0;
								Uint16 subMainMenu[5]; // Used for setting up my main menu;

								clear(3);

								slPrint("Internal Memory", slLocate(5, subMenu++));
								slPrint("External Memory", slLocate(5, subMenu++));

								// Sub Menu options
								slPrint("Format", slLocate(8, subMenu++));
								subMainMenu[subMenuCounter]=0;
								subMenuCounter++;

								slPrint("Delete Save(s)", slLocate(8, subMenu++));
									subMainMenu[subMenuCounter]=1;
									subMenuCounter++;

								slPrint("Copy Save to Internal Memory", slLocate(8, subMenu++));
										subMainMenu[subMenuCounter]=2;
										subMenuCounter++;

								if(floppyMem)
								{
										slPrint("Copy Save to Floppy Memory", slLocate(8, subMenu++));
										subMainMenu[subMenuCounter]=3;
										subMenuCounter++;
								}

								subMenuCounter--; // I need to suptract once

								if(floppyMem)
								{
										slPrint("Floppy Memory", slLocate(5,subMenu++));
								}
								slPrint("CD Memory", slLocate(5,subMenu++));
								slPrint("Credits", slLocate(5,subMenu++));


								// This loop will display the menu, allow the user to move up and down in the menu choices,
							    // and will exit when the user hits A,C, or Start.
							    // Then a switch statement will be executed, using the value of cursor as the key.
								do
								{
									// Erases the old cursor
									slPrint("  ", slLocate(5, subCursor + subOffset));

									// Checks the controller for input
									data = Smpc_Peripheral[0].data;

								   	// Checks if up or down has been pressed, and if it has,
								   	// moves the cursor accordingly. Also wraps the top and bottom,
								    // meaning if you go all the way to the bottom and hit down,
									// the cursor should appear at the top and vice versa.
									if((data & pad_asign[1])== 0)
									{
											subCursor = (subCursor+1)%(subMenuCounter+1);
									}

									if((data & pad_asign[0])== 0)
									{
											subCursor = (subCursor-1)%(subMenuCounter+1);

										   	if(subCursor<0)
										    {
													subCursor = (subMenuCounter);
										    }
									}

									// Writes the cursor to the screen
								  	slPrint(">>", slLocate(5,subCursor + subOffset));

									// Why the slSynch()s? I'm not really sure...
									// I find it slows down the program to a usable speed.
									// There's probably a better method.
									slSynch();
									slSynch();
									slSynch();

								}while((data & pad_asign[7])!= 0 && (data & pad_asign[6])!= 0 && (data & pad_asign[5])!= 0 && (data & pad_asign[4])!= 0); // End of do-while loop

								// Check if user pressed 'B'
								if((data & pad_asign[5])== 0)
								{
										subMainMenu[subCursor] = 4;
								}

								switch(subMainMenu[subCursor]){

									case 0: // Format Memory
									{
											formatDevice(1);
											clear(3);
											break;
									} // Format Memory

									case 1: // Delete Save(s)
									{

										deleteSave(1);
										clear(3);
										break;
									} // Delete Save(s)

									case 2: // Copy to Internal Memory
									{
										backupToBackup(1,0);
										clear(3);
										break;
									} // Internal Memory

									case 3: // Copy to Floppy Memory
									{
											backupToBackup(1,2);
											clear(3);
											break;
									} // Floppy Memory

									case 4: // user hit 'B'
									{
											// do nothing
											break;
									} // user hit 'B'

									default:
									// Should never come here
									slPrint("That's not possible", slLocate(2,11));

					 			}// end of switch statement
							}while((data & pad_asign[5])!= 0); // End of do-while loop


							clear(3);
							break;

				} // External Memory

				case 2: // Floppy Memory Menu
				{
						do{
								Sint32 subCursor = 0;
								Uint16 subMenu = 4;
								Uint16 subOffset = cursor + 5;
								Uint16 subMenuCounter = 0;
								Uint16 subMainMenu[5]; // Used for setting up my main menu;

								clear(3);

								slPrint("Internal Memory", slLocate(5, subMenu++));
								slPrint("External Memory", slLocate(5, subMenu++));
								slPrint("Floppy Memory", slLocate(5, subMenu++));

								// Sub Menu options
								slPrint("Format", slLocate(8, subMenu++));
								subMainMenu[subMenuCounter]=0;
								subMenuCounter++;

								slPrint("Delete Save(s)", slLocate(8, subMenu++));
									subMainMenu[subMenuCounter]=1;
									subMenuCounter++;

								slPrint("Copy Save to Internal Memory", slLocate(8, subMenu++));
									subMainMenu[subMenuCounter]=2;
									subMenuCounter++;

								if(externalMem)
								{
									slPrint("Copy Save to External Memory", slLocate(8, subMenu++));
										subMainMenu[subMenuCounter]=3;
										subMenuCounter++;
								}

								subMenuCounter--;


								slPrint("CD Memory", slLocate(5,subMenu++));
								slPrint("Credits", slLocate(5,subMenu++));


								// This loop will display the menu, allow the user to move up and down in the menu choices,
							    // and will exit when the user hits A,C, or Start.
							    // Then a switch statement will be executed, using the value of cursor as the key.
								do
								{

										// Erases the old cursor
								  		slPrint("  ", slLocate(5, subCursor + subOffset));

										// Checks the controller for input
										data = Smpc_Peripheral[0].data;

								     	// Checks if up or down has been pressed, and if it has,
								       	// moves the cursor accordingly. Also wraps the top and bottom,
								        // meaning if you go all the way to the bottom and hit down,
									    // the cursor should appear at the top and vice versa.
										if((data & pad_asign[1])== 0)
										{
											subCursor = (subCursor+1)%(subMenuCounter+1);
										}

										if((data & pad_asign[0])== 0)
										{
											subCursor = (subCursor-1)%(subMenuCounter+1);

										   	if(subCursor<0)
										    {
												subCursor = (subMenuCounter);
										    }
										}

										// Writes the cursor to the screen
									  	slPrint(">>", slLocate(5,subCursor + subOffset));

										// Why the slSynch()s? I'm not really sure...
										// I find it slows down the program to a usable speed.
										// There's probably a better method.
										slSynch();
										slSynch();
										slSynch();

								}while((data & pad_asign[7])!= 0 && (data & pad_asign[6])!= 0 && (data & pad_asign[5])!= 0 && (data & pad_asign[4])!= 0); // End of do-while loop

								// Check if user pressed 'B'
								if((data & pad_asign[5])== 0)
								{
										subMainMenu[subCursor] = 4;
								}

								switch(subMainMenu[subCursor]){

								case 0: // Format Memory
								{
										formatDevice(2);
										clear(3);
										break;
								} // Format Memory

								case 1: // Delete Save(s)
								{
									deleteSave(2);
									clear(3);
									break;
								} // Delete Save(s)

								case 2: // Copy to Internal Memory
								{
									backupToBackup(2,0);
									clear(3);
									break;
								} // Internal Memory

								case 3: // Copy to External Memory
								{
										backupToBackup(2,1);
										clear(3);
										break;
								} // External Memory

								case 4: // user hit 'B'
								{
									// do nothing
									break;
								} // user hit 'B'


								default:
								// Should never come here
								slPrint("That's not possible", slLocate(2,11));

							 }// end of switch statement


						}while((data & pad_asign[5])!= 0); // End of do-while loop


							clear(3);
							break;

				} // Floppy Memory

				case 3: // CD Memory Menu
				{
					do{
								Sint32 subCursor = 0;
								Uint16 subMenu = 4;
								Uint16 subOffset = cursor + 5;
								Uint16 subMenuCounter = 0;
								Uint16 subMainMenu[5]; // Used for setting up my main menu;

								clear(3);

								slPrint("Internal Memory", slLocate(5, subMenu++));


								if(externalMem)
								{
									slPrint("External Memory", slLocate(5,subMenu++));
								}

								if(floppyMem)
								{
									slPrint("Floppy Memory", slLocate(5,subMenu++));
								}

								slPrint("CD Memory", slLocate(5,subMenu++));

								slPrint("Copy Save to Internal Memory", slLocate(8, subMenu++));
								subMainMenu[subMenuCounter]=0;
								subMenuCounter++;

								if(externalMem)
								{
										slPrint("Copy Save to External Memory", slLocate(8, subMenu++));
											subMainMenu[subMenuCounter]=1;
											subMenuCounter++;
								}

								if(floppyMem)
								{
										slPrint("Copy Save to Floppy Memory", slLocate(8, subMenu++));
												subMainMenu[subMenuCounter]=2;
												subMenuCounter++;
								}

								subMenuCounter--;

								slPrint("Credits", slLocate(5,subMenu++));


								// This loop will display the menu, allow the user to move up and down in the menu choices,
								// and will exit when the user hits A,C, or Start.
								// Then a switch statement will be executed, using the value of cursor as the key.
								do
								{

												// Erases the old cursor
										  		slPrint("  ", slLocate(5, subCursor + subOffset));

												// Checks the controller for input
												data = Smpc_Peripheral[0].data;

										     	// Checks if up or down has been pressed, and if it has,
									        	// moves the cursor accordingly. Also wraps the top and bottom,
										        // meaning if you go all the way to the bottom and hit down,
											    // the cursor should appear at the top and vice versa.
												if((data & pad_asign[1])== 0)
												{
													subCursor = (subCursor+1)%(subMenuCounter+1);
												}

												if((data & pad_asign[0])== 0)
												{
													subCursor = (subCursor-1)%(subMenuCounter+1);

												   	if(subCursor<0)
												    {
														subCursor = (subMenuCounter);
												    }
												}

												// Writes the cursor to the screen
											  	slPrint(">>", slLocate(5,subCursor + subOffset));

												// Why the slSynch()s? I'm not really sure...
												// I find it slows down the program to a usable speed.
												// There's probably a better method.
												slSynch();
												slSynch();
												slSynch();

								 }while((data & pad_asign[7])!= 0 && (data & pad_asign[6])!= 0 && (data & pad_asign[5])!= 0 && (data & pad_asign[4])!= 0); // End of do-while loop

								// Check if user pressed 'B'
								if((data & pad_asign[5])== 0)
								{
													subMainMenu[subCursor] = 3;
								}

								switch(subMainMenu[subCursor]){

												case 0: // Copy save to Internal Memory
												{
																cdToBackup(0);
																clear(3);
																break;
												} // Copy save to Internal Memory

												case 1: // Copy save to External Memory
												{

																cdToBackup(1);
																clear(3);
																break;

												} // Copy save to External Memory


												case 2: // Copy to Floppy Memory
												{
																cdToBackup(2);
																clear(3);
																break;

												} // Copy save to Floppy Memory


												case 3: // User hit B
												{

														// do nothing
														break;

												} // user hit 'B'


												default:
												// Should never come here
												slPrint("That's not possible", slLocate(2,11));

								 			}// end of switch statement


										}while((data & pad_asign[5])!= 0); // End of do-while loop


						clear(3);
						break;

				} // CD Memory Menu

				case 4: // Credits
				{
					clear(3);
					slPrint("Special thanks to:", slLocate(1,4));
					slPrint("Antime, AntiPasta, Charles Doty, Curtis,", slLocate(1,6));
					slPrint("CyberWarriorX, Daniel Eriksson, DBOY", slLocate(1,7));
					slPrint("Djidjo, ExCyber, IBarracudaI,Iceman2k,", slLocate(1,8));
					slPrint("Mal, Nescphp, Printf, Racketboy, RadSil,",slLocate(1,9));
					slPrint("Reinhart, Takashi VBT, Vreuzon, WonderK", slLocate(1,10));
					slPrint("and everybody else keeping the Saturn", slLocate(1,11));
					slPrint("Dev scene alive.", slLocate(1,12));
					slPrint("SegaXtreme's Saturn Programming Contest", slLocate(1,14));
					slPrint("http://www.segaxtreme.com",slLocate(1,15));


					pressStart();
					clear(3);
					break;
				} // Credits

				default:
					// Should never come here
					slPrint("That's not possible", slLocate(2,11));

			 }// end of switch statement

 	  		data = Smpc_Peripheral[0].data; // Checks controller for input

	}while(1); // end of do while loop;


} // SS_Main()

// Clears the screen from y down
void clear(int y)
{
   while(y<28)
	{
	  slPrint("                                            ", slLocate(1, y));
	  y++;
	}

	clearInput();
}

// Displays the text "Press Start" and waits for the user to hit start
void pressStart()
{
   Uint16 start;
   do{
		start = Smpc_Peripheral[0].data; // Checks if start button has been pressed
		slPrint("Press Start", slLocate(15, 25));
		slSynch();
		slSynch();
		slSynch();

    }while((start & pad_asign[7])!= 0);

    clearInput();
}

void clearInput()
{

   Uint16 data;
   do{
		data = Smpc_Peripheral[0].data; // Checks if input device
		slSynch();
		slSynch();
		slSynch();

    }while((data & pad_asign[7])== 0 || (data & pad_asign[6])== 0 || (data & pad_asign[5])== 0 || (data & pad_asign[4])== 0);


}

// Formats the device
void formatDevice(int device)
{
		Sint16 cursor2; // Used for confirming YES\NO
		Uint16 data;

		if(device==0)
		{
					slPrint("Format Internal Memory", slLocate(2,15));
		}
		else if(device==1)
		{
					slPrint("Format External Memory", slLocate(2,15));
		}
		else
		{
					slPrint("Format Floppy Memory", slLocate(2,15));
		}

		// Display warning blah blah blah
		slPrint("WARNING: ALL SAVES WILL BE DELETED", slLocate(2,17));
		slPrint("Are You Sure:", slLocate(2,18));
		slPrint("Yes", slLocate(20,18));
		slPrint("No", slLocate(20, 19));

		// Cursor defaults to "NO"
		cursor2=1;

		clearInput();

		// Checks if the user hits A,B,C, or Start
		// Also moves cursor between Yes and No
		do{

			data = Smpc_Peripheral[0].data; // Checks controller for input

	  		slPrint("   ", slLocate(17,cursor2+18));
			if((data & pad_asign[1])== 0)
				cursor2 = (cursor2+1)%2;
			if((data & pad_asign[0])== 0)
			{
			   cursor2 = (cursor2-1)%2;

 			  if(cursor2<0)
			  {
		             cursor2 = 1;
  			  }
			}

			// You should know what this does by now
			slPrint(">>",slLocate(17, cursor2+18));

			slSynch();
			slSynch();
			slSynch();

		 }while((data & pad_asign[7])!= 0 && (data & pad_asign[6])!= 0 && (data & pad_asign[5])!= 0 && (data & pad_asign[4])!= 0);

		 // User hit 'B'
		 if((data & pad_asign[5])== 0)
		 {
			cursor2=1;
		 }

		clearInput();

		 if(cursor2==0)
		 {
		 	slPrint("Formatting Device....", slLocate(2,20));
			BUP_Format(device); //Formats the device
			slPrint("Done.", slLocate(22,20));
			pressStart();
		 }
}

void backupToBackup(int sourceDevice, int targetDevice)
{
	// Variable Declarations
	Uint16 data; // Used for controller input
	Sint32 cursor2, saveCursor=0; // Used for displaying the cursor
	Sint16 numSaves; // Holds the number of saves
	BupDir saves[20]; // An array of the saves in Backup Memory
	BupStat stat, sttb2; // Used to store the status of the backup devices
	int temp=0; // Used for displaying the saves menu
	char stringBuffer[50];

	clear(3);

	if(sourceDevice==0 && targetDevice==1)
	{
					slPrint("Copy Save(s) from Internal to External", slLocate(2,4));
	}
	else if(sourceDevice==0 && targetDevice==2)
	{
					slPrint("Copy Save(s) from Internal to Floppy", slLocate(2,4));
	}
	else if(sourceDevice==1 && targetDevice==0)
	{
					slPrint("Copy Save(s) from External to Internal", slLocate(2,4));
	}
	else if(sourceDevice==1 && targetDevice==2)
	{
					slPrint("Copy Save(s) from External to Floppy", slLocate(2,4));
	}
	else if(sourceDevice==2 && targetDevice==0)
	{
					slPrint("Copy Save(s) from Floppy to Internal", slLocate(2,4));
	}
	else if(sourceDevice==2 && targetDevice==1)
	{
					slPrint("Copy Save(s) from Floppy to External", slLocate(2,4));
	}


	// Checks if backup device is connected
	// If not, program should return to the main menu
	if(BUP_Stat(sourceDevice, 0, &stat)==BUP_NON)
	{
		   slPrint("Backup Device Not Detected!!!", slLocate(5,6));
		   pressStart();
	}

	// Checks if backup device is formatted
	// If not, program should return to the main menu
	else if(BUP_Stat(sourceDevice, 0, &stat)==BUP_UNFORMAT)
	{
		   slPrint("Backup Device IS Not Formatted!!!", slLocate(5,6));
		   pressStart();
	}

	// Continue with function (1st backup device is connected AND formatted)
	else
	{
			BUP_Stat(sourceDevice, 0, &stat); // stat contains backup information for device
			numSaves = BUP_Dir(sourceDevice,(Uint8 *)"", 50, saves); // Get's number of saves

			// Prints some interesting information

			sprintf(stringBuffer, "%d", stat.freeblock);
		    slPrint(stringBuffer, slLocate(13, 27));
			//slPrintHex(stat.freeblock, slLocate(10, 27));
		    slPrint("Free Blocks: ", slLocate(1,27));

			// Checks if there are any saves on the medium
		   	if(numSaves<=0)
		    {
			      slPrint("There are no saves on the medium.", slLocate(1,6));
			      pressStart();
		    }

			// If there are saves, display them
			else
		    {
			       // Labels
			       slPrint("Filename", slLocate(5, 6));
			       slPrint("Comment", slLocate(20, 6));
			       slPrint("Size", slLocate(34, 6));

		           // Displays all the saves in backup
			       for(temp = 0; temp<numSaves; temp++)
			       {
						slPrint(saves[temp].filename, slLocate(5, temp+7));
		   		        slPrint(saves[temp].comment, slLocate(20, temp+7));
						sprintf(stringBuffer, "%d", saves[temp].blocksize);
				        slPrint(stringBuffer, slLocate(34, temp+7));
					//	slPrintHex(saves[temp].blocksize, slLocate(30, temp+7));
			       }

			 		// Checks if target backup device is connected
			 		// If not, you shouldn't be allowed to copy saves
			 		if(BUP_Stat(targetDevice, 0, &sttb2)==BUP_NON)
			 		{
						slPrint("Second Backup Device Not Detected!!!", slLocate(1, temp+8));
						pressStart();
			 		}

			 		 // Checks if target backup device if formmatted
			 		 // If not, you shouldn't be allowed to copy saves
			 		 else if(BUP_Stat(targetDevice, 0, &sttb2)==BUP_UNFORMAT)
			 		 {
						slPrint("Second Backup Device Not Formated!!!", slLocate(5, temp+8));
						pressStart();
			   		 }

			   		 // Continue with the program
			   		 // 2nd backup device is connected AND formatted
			  		 else
	  		  		{
			  		  	// Displays free blocks information in the bottom right corner
	 					// of the screen
	 					sprintf(stringBuffer, "%d", sttb2.freeblock);
		      	   		slPrint(stringBuffer, slLocate(30, 27));

						//slPrintHex(sttb2.freeblock, slLocate(29, 27));
						slPrint("Free Blocks: ", slLocate(20,27));

						// Displays a cursor next to the saves
						do{
			 					// Erases the old cursor
							  	slPrint("   ", slLocate(2,saveCursor+7));

								// Checks the controller for input
								data = Smpc_Peripheral[0].data;

								// Checks if up or down has been pressed, and if it has,
								// moves the cursor accordingly. Also wraps the top and bottom,
								// meaning if you go all the way to the bottom and hit down,
								// the cursor should appear at the top and vice versa.
								if((data & pad_asign[1])== 0)
									   saveCursor = (saveCursor+1)%(numSaves);
								if((data & pad_asign[0])== 0)
								{
									   saveCursor = (saveCursor-1)%(numSaves);

									   if(saveCursor<0)
										     saveCursor = numSaves-1;
								}

								// Writes the cursor to the screen
								slPrint(">>", slLocate(2,saveCursor+7));

								slSynch();
								slSynch();
								slSynch();

						  }while((data & pad_asign[7])!= 0 && (data & pad_asign[6])!= 0 && (data & pad_asign[5])!= 0 && (data & pad_asign[4])!= 0); // End of do-while loop

						// Check if user did not hit B
						if((data & pad_asign[5])!= 0)
						{
							Sint32 test; // test will hold the return value from BUP_Write

							// Checks if there's enough space for the save
							if(sttb2.freeblock > saves[saveCursor].blocksize)
							{

							     BupDir writetb; // This holds all of the save information
							     Uint8*  savePointer; // Pointer to the save to be copied

							     // Puts the proper information in writetb
							     strcpy((char *)writetb.filename, saves[saveCursor].filename);
	    			  		     strcpy((char *)writetb.comment, saves[saveCursor].comment);
							     writetb.language = saves[saveCursor].language;
							     writetb.datasize = saves[saveCursor].datasize;
							     writetb.date = saves[saveCursor].date;

					             // Reads the save into memory, savePointer points to it
							     BUP_Read(sourceDevice, saves[saveCursor].filename, savePointer);

							     // This should write the save file, unless file already exists
							     test = BUP_Write(targetDevice, &writetb, savePointer, ON);

							     // What to do if there's already a file with the same name
							     if(test==BUP_FOUND)
							     {
									// Blah
									slPrint("WARNING: OVERWRITE EXISTING", slLocate(1,numSaves + 9));
									slPrint(saves[saveCursor].filename,slLocate(30,9+numSaves));
									slPrint("Are You Sure:", slLocate(1,numSaves + 10));
									slPrint("Yes", slLocate(20, numSaves + 10));
									slPrint("No", slLocate(20, numSaves + 11));
									cursor2=1;

									clearInput();

									// Checks if the user hits A,B,C, or Start
									// Also moves cursor between Yes and No
									do{

										data = Smpc_Peripheral[0].data; // Checks controller for input

								  		slPrint("   ", slLocate(17,cursor2+10+numSaves));

										if((data & pad_asign[1])== 0)
										{
											cursor2 = (cursor2+1)%2;
										}
										if((data & pad_asign[0])== 0)
										{
								   			cursor2 = (cursor2-1)%2;
								    		if(cursor2<0)
											{
									             cursor2 = 1;
											}
										}

										slPrint(">>",slLocate(17, cursor2+10+numSaves));

										slSynch();
										slSynch();
										slSynch();

								     }while((data & pad_asign[7])!= 0 && (data & pad_asign[6])!= 0 && (data & pad_asign[5])!= 0 && (data & pad_asign[4])!= 0);


									   if((data & pad_asign[5])== 0)
									   {
									 		cursor2=1;

										}

									  if(cursor2==0)
							  		  {
										  	clear(cursor2+10+numSaves);
								    		slPrint("Copying Save....", slLocate(1,12+numSaves));
											BUP_Write(targetDevice, &writetb, savePointer, OFF);
									        slPrint("Done.", slLocate(16,12+numSaves));
									  }

										clearInput();


									} // End of if test==found

									// What to do if write executed properly
									else
									{
				    					slPrint("Copying Save....", slLocate(1,8+numSaves));
								        slPrint("Done.", slLocate(16,8+numSaves));
								        pressStart();
									}
								} // end of if there is enough space

								// What to do if there's isn't enough space
								else
								{
									  slPrint("There is not enough free space on the medium", slLocate(1, 8+numSaves));
									  slPrint("Delete some files and try again.", slLocate(1,9+numSaves));
									  pressStart();
								}


						} // end of user did not hit B
	   		   } // end of else second backup device is connected AND formatted
		    } // first backup device has saves
	 	 } // end of else first backup device is connected AND formatted
} // end of backup to backup

// Gives user a menu to select which save to delete
void deleteSave(int device)
{
		// Variable Declartions
		Uint16 data; // Used for controller input
		Sint32 cursor2, saveCursor=0; // Used for displaying the cursor
		Sint16 numSaves; // Holds the number of saves
	    BupDir saves[20]; // An array of the saves in Backup Memory
	    BupStat stat; // Used to hold the device information
		int temp=0; // Used for displaying the saves menu
		char stringBuffer[50];

		clear(3); // Clears the screen

		if(device==0)
		{
			slPrint("Delete Internal Memory Save(s)", slLocate(2,4));
		}
		else if(device==1)
		{
			slPrint("Delete External Memory Save(s)", slLocate(2,4));
		}
		else
		{
			slPrint("Delete Floppy Memory Save(s)", slLocate(2,4));
		}

		// Checks if backup device is connected
		// If it's not, program should return to main menu
		if(BUP_Stat(device, 0, &stat)==BUP_NON)
		{
		    slPrint("Backup Device Not Detected!!!", slLocate(2,6));
		    pressStart();
		}

		// Backup device is connected
		else
		{

		    BUP_Stat(device, 0, &stat); // I think this initializes stat to hold the information for device
            numSaves = BUP_Dir(device,(Uint8 *)"", 50, saves); // numSaves holds the number of saves on device

		    // Prints some interesting information
		    sprintf(stringBuffer, "%d", stat.freeblock);
			slPrint(stringBuffer, slLocate(14, 27));

			sprintf(stringBuffer, "%d", numSaves);
		    slPrint(stringBuffer, slLocate(32, 27));

		    slPrint("Free Blocks: ", slLocate(2,27));
		    slPrint("Save Games: ", slLocate(20,27));

		    // Checks if there are any saves on the medium to display
		    if(numSaves<=0)
		    {
		      slPrint("There are no saves on the medium.", slLocate(2,6));
		      pressStart();
		    }

		    // Backup device is connected AND has saves
		    else
		    {

		       // Labels
		       slPrint("Filename", slLocate(5, 6));
		       slPrint("Comment", slLocate(20, 6));
		       slPrint("Size", slLocate(34, 6));

		       // Displays all the saves in backup
		       for(temp = 0; temp<numSaves; temp++)
		       {
				   slPrint(saves[temp].filename, slLocate(5, temp+7));
				   slPrint(saves[temp].comment, slLocate(20, temp+7));
				   sprintf(stringBuffer, "%d", saves[temp].blocksize);
		      	   slPrint(stringBuffer, slLocate(34, temp+7));

		       }

		    	// Displays a cursor next to all the menu options
		    	// do-while loop breaks when user selects
		    	do{

		 			// Erases the old cursor
		  			slPrint("   ", slLocate(2,saveCursor+7));

					// Checks the controller for input
					data = Smpc_Peripheral[0].data;

		    	    // Checks if up or down has been pressed, and if it has,
					// moves the cursor accordingly. Also wraps the top and bottom,
					// meaning if you go all the way to the bottom and hit down,
					// the cursor should appear at the top and vice versa.
					if((data & pad_asign[1])== 0)
					{
					   saveCursor = (saveCursor+1)%(numSaves-1);
					}
					if((data & pad_asign[0])== 0)
					{
					   saveCursor = (saveCursor-1)%(numSaves-1);
					   if(saveCursor<0)
					   {
					      saveCursor = numSaves-1;
					   }
					}

					// Writes the cursor to the screen
		  			slPrint(">>", slLocate(2,saveCursor+7));

					slSynch();
					slSynch();
					slSynch();

		    	}while((data & pad_asign[7])!= 0 && (data & pad_asign[6])!= 0 && (data & pad_asign[5])!= 0 && (data & pad_asign[4])!= 0); // End of do-while loop

		    	// If User didn't press 'B'
		    	if((data & pad_asign[5])!= 0)
		    	{
					// Prompt user blah blah blah
					slPrint("WARNING:             WILL BE DELETED", slLocate(1,numSaves + 8));
					slPrint(saves[saveCursor].filename,slLocate( 10,8+numSaves));
					slPrint("Are You Sure:", slLocate(1,numSaves + 9));
					slPrint("Yes", slLocate(20, numSaves + 9));
					slPrint("No", slLocate(20, numSaves + 10));

					// Sets cursor to the second position, "NO" by default
					cursor2=1;

					clearInput();

					// Checks if the user hits A,B,C, or Start
					// Also moves cursor between Yes and No
					do{

						data = Smpc_Peripheral[0].data; // Checks controller for input

						slPrint("   ", slLocate(17,cursor2+9+numSaves)); // erases cursor

						// Mod operations for position the cursor
						if((data & pad_asign[1])== 0)
						{
							cursor2 = (cursor2+1)%2;
						}

						if((data & pad_asign[0])== 0)
						{
  						      cursor2 = (cursor2-1)%2;

				 			  if(cursor2<0)
							  {
		             				cursor2 = 1;
			 				  }
						}


						slPrint(">>",slLocate(17, cursor2+9+numSaves)); // displays the cursor

						slSynch();
						slSynch();
						slSynch();


					}while((data & pad_asign[7])!= 0 && (data & pad_asign[6])!= 0 && (data & pad_asign[5])!= 0 && (data & pad_asign[4])!= 0);

					if((data & pad_asign[5])== 0)
					{
						cursor2=1;
					}

					clear(cursor2+8+numSaves);

					// Cursor2==0 means that the user selected "Yes"
					// Delete should proceed
					if(cursor2==0)
					{
						    slPrint("Deleting Save....", slLocate(1,13+numSaves));
						    BUP_Delete(device,saves[saveCursor].filename ); //Deletes the save
						    slPrint("Done.", slLocate(21,13+numSaves));
						    pressStart();
					}

	    		} // If user didn't select B
	     } // Numsaves > 0

      } // end of else backup is connected

} // End of delete save function

// Copies save from CD to backup device
void cdToBackup(int device)
{
		// Variables needed for CD functions
		Sint32 ndir;
		CDHN cdhn;
		CDKEY key[2];
		CDBUF buf[2];
		Sint32 stats;
		Sint32 len[2];
		BupStat stat;

		// Other variables
		int temp; // used in the for loop
		char fileName[12]; // holds the filename
		Sint32 fileSize[2], blockSize[2]; // I needed pointers for the file size and the block size of each save
		Sint32 saveCursor=0, cursor2; // cursors
		Uint16 data; // controller
		char stringBuffer[50];

		// Initializes the cd
		ndir=slCdInit(MAX_FILE, dirwork); // initializes cd
		slCdChgDir("SAVES"); // changes to the SAVES directory on the cd

		clear(3);

		if(device==0)
		{
				slPrint("Copy Save(s) from CD to Internal", slLocate(2,4));
		}
		else if(device==1)
		{
				slPrint("Copy Save(s) from CD to External", slLocate(2,4));
		}
		else
		{
				slPrint("Copy Save(s) from CD to Floppy", slLocate(2,4));
		}

        // Labels
        slPrint("Filename", slLocate(5, 6));
		slPrint("Bytes", slLocate(20, 6));
		slPrint("Blocks", slLocate(30, 6));

		// Displays up to the first 15 saves in the backup
		for(temp = 2; temp<17 && GFS_IdToName(temp)!=NULL; temp++)
		{

		   // fileSize points to the filesize of the file
		   GFS_GetFileInfo(GFS_Open(temp), NULL, NULL, fileSize, NULL);

		   // blockSize points to the number of blocks the file will take up
		   getBlockSize(*fileSize, blockSize);

		   // fileName points to the the correct filename
		   renameFile(temp, fileName);


		   sprintf(stringBuffer, "%d", *fileSize);
		   slPrint(stringBuffer, slLocate(20, temp+5));

		   sprintf(stringBuffer, "%d", *blockSize);
		   slPrint(stringBuffer, slLocate(30, temp+5));

		   //slDispHex(*fileSize, slLocate(13, temp+5));	// print file size
		   //slDispHex(*blockSize, slLocate(25, temp+5)); // print block size
		   slPrint(fileName, slLocate(5,temp+5)); // print file name
		}

		// Checks if target backup device is connected
		if(BUP_Stat(device, 0, &stat)==BUP_NON)
		{
			slPrint("Target Backup Device Not Detected!!!", slLocate(2, 4));
	  	}

		// Checks if target backup device if formmatted
		else if(BUP_Stat(device, 0, &stat)==BUP_UNFORMAT)
		{
			slPrint("Target Backup Device Not Formated!!!", slLocate(2, 4));
		}

		// Continue with program
		else
		{
			// Displays free blocks information in the bottom right corner
	 		// of the screen
	 		sprintf(stringBuffer, "%d", stat.freeblock);
			slPrint(stringBuffer, slLocate(31, 27));
			slPrint("Free Blocks: ", slLocate(20,27));

	        do{

		  	 	// Erases the old cursor
  	  	        slPrint("   ", slLocate(2,saveCursor+7));

				// Checks the controller for input
	  			data = Smpc_Peripheral[0].data;

				// Checks if up or down has been pressed, and if it has,
		        // moves the cursor accordingly. Also wraps the top and bottom,
				// meaning if you go all the way to the bottom and hit down,
				// the cursor should appear at the top and vice versa.
				if((data & pad_asign[1])== 0)
				{
					saveCursor = (saveCursor+1)%(temp-2);


				}

				if((data & pad_asign[0])== 0)
				{
				   saveCursor = (saveCursor-1)%(temp-2);

				   if(saveCursor<0)
				     saveCursor = temp-3;
				}

				// Writes the cursor to the screen
	  			slPrint(">>", slLocate(2,saveCursor+7));

				slSynch();
				slSynch();
				slSynch();

			  }while((data & pad_asign[7])!= 0 && (data & pad_asign[6])!= 0 && (data & pad_asign[5])!= 0 && (data & pad_asign[4])!= 0); // End of do-while loop


			  // User didn't press B
			  if((data & pad_asign[5]) != 0)
			  {
					Sint32 test;

					// fileSize points to the filesize of the file
					GFS_GetFileInfo(GFS_Open(saveCursor+2), NULL, NULL, fileSize, NULL);

			        // blockSize points to the number of blocks the file will take up
			        getBlockSize(*fileSize, blockSize);

					renameFile(saveCursor+2, fileName);


					// Checks if there's enough space for the save
					if(stat.freeblock > *blockSize)
					{

						  BupDir writetb; // This holds all of the save information
						  Uint8*  savePointer; // Pointer to the save to be copied

						  // Puts the proper information in writetb
						  strcpy((char *)writetb.filename, fileName);
						  strcpy((char *)writetb.comment, "SLINGA"); // and you thought I didn't write my name anywhere :Þ
						  writetb.language = BUP_ENGLISH;
						  writetb.datasize = *fileSize;

						  // Crazy cd functions, don't ask me to explain what's going on...
						  key[0].cn=key[0].sm=key[0].ci=CDKEY_NONE;
						  key[1].cn=CDKEY_TERM;
						  cdhn=slCdOpen(GFS_IdToName(saveCursor+2), key); // opens cd
						  buf[0].type=CDBUF_COPY;
						  buf[0].trans.copy.addr=readbuf;
						  buf[0].trans.copy.unit=CDBUF_FORM1;
						  buf[0].trans.copy.size=READSECT;
						  buf[1].type=CDBUF_TERM;
						  slCdLoadFile(cdhn,buf); // loads file from cd

						  while(stats!=CDSTAT_COMPLETED)
						  {
									slSynch();
									stats=slCdGetStatus(cdhn, len);
									if(stats==CDSTAT_COMPLETED) break;
						  }


						  // This should write the save file, unless file already exists
				          test = BUP_Write(device, &writetb, readbuf, ON);

						  // What to do if there's already a file with the same name
						  if(test==BUP_FOUND)
						  {
								slPrint("WARNING: OVERWRITE EXISTING", slLocate(1,temp + 6));
								slPrint(fileName,slLocate(29,temp +6));
								slPrint("Are You Sure:", slLocate(1,temp + 7));
								slPrint("Yes", slLocate(20, temp + 7));
								slPrint("No", slLocate(20, temp + 8));

								cursor2=1;

								clearInput();

								// Checks if the user hits A,B,C, or Start
								// Also moves cursor between Yes and No
								do{
										data = Smpc_Peripheral[0].data; // Checks controller for input

								  		slPrint("   ", slLocate(17,cursor2+temp+7));

										if((data & pad_asign[1])== 0)
										{
													cursor2 = (cursor2+1)%2;
										}
										if((data & pad_asign[0])== 0)
										{

								   			cursor2 = (cursor2-1)%2;
						 			        if(cursor2<0)
											{
												   cursor2 = 1;
											}
										}

										slPrint(">>",slLocate(17, cursor2+7+temp));

										slSynch();
										slSynch();
										slSynch();


								  }while((data & pad_asign[7])!= 0 && (data & pad_asign[6])!= 0 && (data & pad_asign[5])!= 0 && (data & pad_asign[4])!= 0);


								// User hit B
								if((data & pad_asign[5])== 0								  )
								{
									cursor2 = 1;
								}

								// User hit "Yes", proceed with overwrite
								if(cursor2==0)
							  	{
										slPrint("Copying Save....", slLocate(2,10+temp));
										BUP_Write(device, &writetb, readbuf, OFF);
										slPrint("Done.", slLocate(17,10+temp));
										pressStart();
						        }

						        clearInput();

							  } // end if test==bup_found

							  // What to do if write executed properly
							  else
							  {
							    		slPrint("Copying Save....", slLocate(1,temp+6));
								        slPrint("Done.", slLocate(16,temp+6));
								        pressStart();
							  }

						    } // If there's enough space

						    // What to do if there's isn't enough space
						    else
						    {
								  slPrint("There is not enough free space on the medium", slLocate(2, temp+7));
								  slPrint("Delete some files and try again.", slLocate(2,temp+8));
								  pressStart();
				 		    }


						} // User Selected a Save, didn't hit B

		} // Continue with Program

} // End of cdToBackup Function

// Reads a 8.3 filename from the cd and returns it as 11 characters
// I can't believe this works
void renameFile(Sint32 fid, char* fnamePointer)
{
	char* temp;
	temp = GFS_IdToName(fid);

	// First 8 characters should be the same
	fnamePointer[0] = temp[0];
	fnamePointer[1] = temp[1];
	fnamePointer[2] = temp[2];
	fnamePointer[3] = temp[3];
	fnamePointer[4] = temp[4];
	fnamePointer[5] = temp[5];
	fnamePointer[6] = temp[6];
	fnamePointer[7] = temp[7];

	// Skips the period
	fnamePointer[8] = temp[9];
	fnamePointer[9] = temp[10];
	fnamePointer[10] = temp[11];
	fnamePointer[11] = temp[12];

}

// Need to double check if this is returning the correct values
void getBlockSize(Sint32 bytes, Sint32* blocks)
{
	double temp;
	temp = (bytes + 32)/64;
	*blocks = (temp);
}








