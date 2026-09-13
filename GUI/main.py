import customtkinter as ctk
from Contract.serial_layer import SerialLayer
from enum import Enum
import time 

class BoardState(Enum): 
    """!@brief A enum used to represent the state of the microcontroller. 
    """
    WAITING = 1
    EXECUTING = 2

class CANController(ctk.CTk):
    """!@brief A class for setting up and controlling the GUI. Inherits from ctk.CTk. 
    """
    _PAD = 12
    _GROUP_GAP = 24
    _SIDEBAR_W = 260
    _WIDGET_H = 36

    def __init__(self):
        super().__init__()
        self._board_state = BoardState.EXECUTING 
        self._serial = SerialLayer()
        #Basic window setup. 
        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("dark-blue")
        self.geometry("%dx%d+0+0" % (self.winfo_screenwidth(), self.winfo_screenheight())) 
        self.minsize(640, 400)
        self.title("USBCANTool Controller") 

        self.grid_rowconfigure(0, weight = 1)
        self.grid_columnconfigure(0, weight = 0, minsize = self._SIDEBAR_W)
        self.grid_columnconfigure(1, weight = 1)

        #Sidebar holds every control so they share one left margin and one width.
        sidebar = ctk.CTkFrame(self, corner_radius = 0, fg_color = "transparent")
        sidebar.grid(row = 0, column = 0, sticky = "nsew")
        sidebar.grid_columnconfigure(0, weight = 1)
        #Trailing spacer row keeps the controls pinned to the top as the window grows.
        sidebar.grid_rowconfigure(7, weight = 1)

        #Menu Boxes 
        #Com Port Menu Box 
        #No default values because they are loaded periodically. 
        self._COM_menu = ctk.CTkOptionMenu(sidebar, height = self._WIDGET_H, anchor = "center",  
                                         command = None)
        self._last_COM_values = []
        self._COM_menu.grid(row = 0, column = 0, sticky = "ew", padx = self._PAD, pady = (self._PAD, 4))
        #COM Port Use Button
        COM_use_but = ctk.CTkButton(sidebar, text = "Use COM Port", height = self._WIDGET_H, command = self._use_COM_port)
        COM_use_but.grid(row = 1, column = 0, sticky = "ew", padx = self._PAD, pady = (0, self._GROUP_GAP))

        #UDS Options Menu Box 
        #TO-DO: Will need to load these values from another file that contains all of the avilable UDS commands. 
        self._UDS_menu = ctk.CTkOptionMenu(sidebar, values=["TESTERPRESENT", "VIN"], height = self._WIDGET_H, anchor = "center",  
                                         command = None)
        self._UDS_menu.set("TESTERPRESENT") 
        self._UDS_menu.grid(row = 2, column = 0, sticky = "ew", padx = self._PAD, pady = (0, 4))
        #UDS Send Button 
        UDS_send_but = ctk.CTkButton(sidebar, text = "Send UDS Command", height = self._WIDGET_H, command = self._send_UDS)
        UDS_send_but.grid(row = 3, column = 0, sticky = "ew", padx = self._PAD, pady = (0, self._PAD))

        #CAN Send Box
        text_font = ctk.CTkFont(family = "Times New Roman", size = 24) 
        self._CAN_box = ctk.CTkEntry(master = sidebar, 
                                       font = text_font, 
                                       height = self._WIDGET_H, 
                                       corner_radius = 6, 
                                       text_color = "white", 
                                        )

        self._CAN_box.grid(row = 4, column = 0, sticky = "ew", padx = self._PAD, pady = (self._PAD, 4))
        def validate_can_box(e) -> None: 
                txt = self._CAN_box.get() 
                if(len(txt) > 8): 
                    self._CAN_box.delete(8, ctk.END) 
                    self._term_write("Can messages must be a maximum of 8 bytes!")

        self._CAN_box.bind('<KeyRelease>', validate_can_box)

        #CAN Send Button 
        CAN_send_but = ctk.CTkButton(sidebar, text = "Send CAN Message", height = self._WIDGET_H, command = self._send_CAN)
        CAN_send_but.grid(row = 5, column = 0, sticky = "ew", padx = self._PAD, pady = (0, self._PAD))

        #Terminal box initialization 
        #Custom font for the terminal  
        self._term_index = 1 
        #No fixed width/height: the grid cell sizes it, so it tracks the window.
        self._term = ctk.CTkTextbox(master = self, 
                                    font = text_font, 
                                    corner_radius = 6, 
                                    text_color = "white", 
                                    state = "disabled"
                                    ) 
        #Same top and bottom padding as the sidebar, so the terminal's top edge lines
        #up with the first control rather than floating in the middle of the window.
        self._term.grid(row = 0, column = 1, sticky = "nsew", padx = (0, self._PAD), pady = self._PAD)

        #CAN Receive Button
        self._CAN_receive_but = ctk.CTkButton(sidebar, text = "Start Receiving CAN Messages", height = self._WIDGET_H, command = self._receive_CAN)
        self._CAN_receive_but.grid(row = 6, column = 0, sticky = "ew", padx = self._PAD, pady = (0, self._PAD))

        self.protocol("WM_DELETE_WINDOW", self._on_close)
        self._process_serial()
        self._update_COM_menu()

    def _on_close(self) -> None:
        #Release the COM port before the window/interpreter goes away.
        self._serial.close()
        self.destroy()

    #Console Functions
    def _term_write(self, text : str) -> None:
        #Enabling the textbox here to write text. 
        self._term.configure(state = "normal")
        self._term.insert("end", text + "\n")
        self._term.see("end")
        self._term.configure(state = "disabled")

    #Periodic functions 
    #Periodic Menu Functions
    def _update_COM_menu(self) -> None: 
        #Prevent the box from updating too frequently. 
        COM_values = self._serial.get_com_list()
        if(COM_values != self._last_COM_values): 
            self._last_COM_values = COM_values
            #This is still ok if .get_com_list() returns nothing. 
            self._COM_menu.configure(values = COM_values)
            #Must do some checking here for an empty list. 
            self._COM_menu.set("None Detected" if not COM_values else COM_values[0])
        self.after(1000, self._update_COM_menu)  

    #Periodic serial read function 
    def _process_serial(self) -> None: 
        if(self._serial.bytes_ready): 
            txt = self._serial.read_line()
            if(txt == "Enter CMD" or ("ERROR:" in txt)): 
                self._board_state = BoardState.WAITING
            if(txt != "Enter CMD"): 
                self._term_write(txt) 
        self.after(100, self._process_serial) 

    #Button Functions
    def _use_COM_port(self) -> None: 
        COM_value = self._COM_menu.get()
        if(COM_value != "None Detected"): 
            #Doing this to catch any errors produced by pyserial. 
            try: 
                self._serial.connect(COM_value)
                self._term_write(f"Connected to {COM_value}")
                time.sleep(0.050)
                self._board_state = BoardState.WAITING 
            except Exception as e:
                self._term_write(f"Problem using that COM port, try another!")
        else: 
            self._term_write("Please select a valid COM port!")

    #Send functions(called after a button is pressed) 
    def _send_check(self) -> bool: 
        if self._serial.is_connected:
            if self._board_state != BoardState.WAITING: 
                self._term_write("Please wait until the board is finished executing its current command!")
            else: 
                return True 
        else: 
            self._term_write("Please connect to a COM port before sending a command!")
        return False  
         
    def _send_UDS(self) -> None: 
        if self._send_check(): 
            self._board_state = BoardState.EXECUTING
            self._term_write("Sending UDS:" + self._UDS_menu.get())
            self._serial.send("UDS:" + self._UDS_menu.get())

    def _send_CAN(self) -> None: 
        if self._send_check(): 
            self._board_state = BoardState.EXECUTING
            self._term_write("Sending CANS:" + self._CAN_box.get())
            self._serial.send("CANS:" + self._CAN_box.get())

    def _receive_CAN(self) -> None: 
        if self._CAN_receive_but.cget("text") == "Start Receiving CAN Messages": 
            if self._send_check(): 
                self._board_state = BoardState.EXECUTING
                self._term_write("Sending CANR:")
                self._serial.send("CANR:")
                self._CAN_receive_but.configure(text = "Stop Receiving CAN Messages")
        else: 
            #We can send any character here. 
            self._term_write("S") 
            self._term_write("Stopping CAN Receiving") 
            self._CAN_receive_but.configure(text = "Start Receiving CAN Messages")

def main() -> None:
    controller = CANController()
    try:
        controller.mainloop()
    finally:
        controller._serial.close()
    return

if __name__ == '__main__': 
    main() 