import customtkinter as ctk

class CANController(ctk.CTk): 
    def __init__(self): 
        super().__init__() 
        #Basic window setup. 
        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("dark-blue")
        self.geometry("%dx%d+0+0" % (self.winfo_screenwidth(), self.winfo_screenheight())) 
        self.title("USBCANTool Controller") 

        #Console initialization 
        self._term_index = 0 
        self._term = ctk.CTkTextbox(master = self, 
                                    height = 800, 
                                    width = 1400, 
                                    corner_radius = 0, 
                                    text_color = "white", 
                                    state = "disabled"

                                    ) 
        self._term.place(relx=0.5, rely=0.5, anchor="center")
        self._term.insert("0.0", "Terminal\n" * 50)

    #Console Functions 
    def _term_write(self, text : str) -> None: 
        self._term.insert(self._term_index, text)
        self._term_index += 1  

def main() -> None: 
    controller = CANController()
    controller.mainloop() 
    return 

if __name__ == '__main__': 
    main() 