class LastScreen 
{
    public:
        int name;
        LastScreen();
        ~LastScreen();
        void set(int name);
        int prev();
    private:
        LastScreen *prevptr;   
};