import customtkinter as ctk
from Contract.serial_layer import SerialLayer

class CANController(ctk.CTk):
    def __init__(self):
        super().__init__()
        self._serial = SerialLayer()
        #Basic window setup. 
        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("dark-blue")
        self.geometry("%dx%d+0+0" % (self.winfo_screenwidth(), self.winfo_screenheight())) 
        self.title("USBCANTool Controller") 

        #Useful variable for standardization, scaled to the screen size
        self._widget_w = self.winfo_screenwidth() * 0.166
        self._widget_h = self.winfo_screenheight() * 0.045

        #Terminal box initialization 
        #Custom font for the terminal  
        term_font = ctk.CTkFont(family = "Times New Roman", size = 24) 
        self._term_index = 1 
        self._term = ctk.CTkTextbox(master = self, 
                                    font = term_font, 
                                    height = self.winfo_screenheight() / 1.5, 
                                    width = self.winfo_screenwidth() / 1.5, 
                                    corner_radius = 0, 
                                    text_color = "white", 
                                    state = "disabled"
                                    ) 
        self._term.place(relx = 0.5, rely = 0.5, anchor="center")
        print(self._serial.get_com_list())
        #Menu Boxes 
        #Com Port Menu Box 
        #No default values because they are loaded periodically. 
        self._COM_menu = ctk.CTkOptionMenu(self, width = self._widget_w, height = self._widget_h, anchor = "s",  
                                         command = None)
        self._last_COM_values = []
        self._COM_menu.place(relx = 0, rely = 0.160, anchor = "nw")
        #UDS Options Menu Box 
        #TO-DO: Will need to load these values from another file that contains all of the avilable UDS commands. 
        self._UDS_menu = ctk.CTkOptionMenu(self, values=["option 1", "option 2"], width = self._widget_w, height = self._widget_h, anchor = "s",  
                                         command = None)
        self._UDS_menu.set("option 1") 
        self._UDS_menu.place(relx = 0, rely = 0.45, anchor = "nw")

        #Buttons 
        #COM Port Use Button
        COM_use_but = ctk.CTkButton(self, text = "Use COM Port", width = self._widget_w, height = self._widget_h, command = self.use_COM_port)
        COM_use_but.place(relx = 0, rely = 0.205, anchor = "nw")
        #UDS Send Button 
        UDS_send_but = ctk.CTkButton(self, text = "Send UDS Command", width = self._widget_w, height = self._widget_h, command = self.send_UDS)
        UDS_send_but.place(relx = 0, rely = 0.495, anchor = "nw")

        self._update_COM_menu() 

    #Console Functions
    def _term_write(self, text : str) -> None:
        #Enabling the textbox here to write text. 
        self._term.configure(state = "normal")
        self._term.insert("end", text + "\n")
        self._term.see("end")
        self._term.configure(state = "disabled")

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
        self.after(500, self._update_COM_menu) 
        return 

    #Button Functions
    def use_COM_port(self) -> None: 
        COM_value = self._COM_menu.get()
        if(COM_value != "None Detected"): 
            #Doing this to catch any errors produced by pyserial. 
            try: 
                self._serial.connect(COM_value)
                self._term_write(f"Connected to {COM_value}")
            except Exception as e:
                self._term_write("Problem using that COM port, try another!")
        else: 
            self._term_write("Please select a valid COM port!")
        return 
         
    def send_UDS(self) -> None: 
        self._term_write(self._UDS_menu.get())
        return 



def main() -> None: 
    controller = CANController()
    controller.mainloop() 
    return 

if __name__ == '__main__': 
    main() 