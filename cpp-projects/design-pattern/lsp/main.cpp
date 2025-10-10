#include <iostream>
using namespace std;

///////////////////////////////////////里氏替换原则///////////////////////////////////////////////////
// 基类
class Calculator {
public:
    int calculate(int n1, int n2) {
        return n1 + n2;
    }
};

//继承类
class BadCalculator : public Calculator {
public:
    // 重写父类方法,但是修改了父类方法的行为，本来是加法，变成了减法
    int calculate(int n1, int n2) {
        return n1 - n2;
    }
};

class GoodCalculator : public Calculator {
public:
    // 子类实现自己特有的方法
    int add(int n1, int n2) {
        return n1 + n2;
    }

    int sub(int n1, int n2) {
        return n1 - n2;
    }
};

///////////////////////////////////////接口隔离原则///////////////////////////////////////////////////
class IPrinter {
public:
    virtual void print(const string& doc) = 0;
    virtual ~IPrinter(){}
};

class IScanner {
public:
    virtual void scan(const string& doc) = 0;
    virtual ~IScanner(){}
};

class IFax {
public:
    virtual void fax(const string& doc) = 0;
    virtual ~IFax(){}
};

// --- 设备实现类 ---
// 1️⃣ 仅实现打印机接口
class SimplePrinter : public IPrinter {
public:
    void print(const string& doc) {
        cout << "SimplePrinter Printing: " << doc << endl;
    }
};

// 2️⃣ 仅实现扫描仪接口
class SimpleScanner : public IScanner {
public:
    void scan(const string& doc) {
        cout << "SimpleScanner Scanning: " << doc << endl;
    }
};

// 3️⃣ 多功能一体机 —— 组合多个接口
class MultiFunctionMachine : public IPrinter, public IScanner, public IFax {
public:
    void print(const string& doc) {
        cout << "MultiFunctionMachinePrinting: " << doc << endl;
    }

    void scan(const string& doc) {
        cout << "MultiFunctionMachine Scanning: " << doc << endl;
    }

    void fax(const string& doc) {
        cout << "MultiFunctionMachine Faxing: " << doc << endl;
    }
};

/////////////////////////////////////////////依赖倒置原则////////////////////////////////////////////////////
// 抽象接口：通知者
class INotifier {
public:
    virtual void send(const string& message) = 0;
    virtual ~INotifier() {}
};

// 低层模块：具体实现1 - 短信通知
class SMSNotifier : public INotifier {
public:
    void send(const string& message) {
        cout << "Sending SMS: " << message << endl;
    }
};

// 低层模块：具体实现2 - 邮件通知
class EmailNotifier : public INotifier {
public:
    void send(const string& message) {
        cout << "Sending Email: " << message << endl;
    }
};
    
class MessageService {
    INotifier* notifier;
public:
    //构造函数依赖注入（由外部传入具体实现）
    MessageService(INotifier* notifier) : notifier(notifier) {}

    //发送通知
    void sendMessage(const string& message) {
        notifier->send(message);
    }
};

//子类必须实现父类的所有抽象方法，但是不能覆盖父类的非抽象方法
int main() {
    Calculator c1;
    BadCalculator c2;

    int sum1 = c1.calculate(5, 10);//能运行，行为正常
    cout << "c1.calculate(5,10) = " << sum1 << endl;

    int sum2 = c2.calculate(5, 10);//能运行，但是行为不正常，破坏了父类的行为
    cout << "c2.calculate(5,10) = " << sum2 << endl;

    GoodCalculator goodCalc;
    int addResult = goodCalc.add(5, 10);
    cout << "goodCalc.add(5,10) = " << addResult << endl;
    int subResult = goodCalc.sub(5, 10);
    cout << "goodCalc.sub(5,10) = " << subResult << endl;

    cout << "--------------------------------" << endl;

    //单纯的打印机
    SimplePrinter printer;
    printer.print("Hello, World!");

    //单纯的扫描器
    SimpleScanner scanner;
    scanner.scan("Hello, World!");

    //多功能一体机
    MultiFunctionMachine multiFunctionMachine;
    multiFunctionMachine.print("Hello, World!");
    multiFunctionMachine.scan("Hello, World!");
    multiFunctionMachine.fax("Hello, World!");

    cout << "--------------------------------" << endl;

    // 用短信方式发送
    SMSNotifier* smsNotifier = new SMSNotifier();
    MessageService messageService(smsNotifier);
    messageService.sendMessage("Hello, World!");

    // 用邮件方式发送
    EmailNotifier* emailNotifier = new EmailNotifier();
    MessageService messageService2(emailNotifier);
    messageService2.sendMessage("Hello, World!");
    return 0;
}