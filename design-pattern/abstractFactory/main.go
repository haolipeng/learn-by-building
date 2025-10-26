package main

import "fmt"

// 定义产品接口
// 椅子
type Chair interface {
	SitOn()           //坐
	GetStyle() string //风格
}

type Table interface {
	PutOn()           //放
	GetStyle() string //风格
}

// 家具工厂接口
type FurnitureFactory interface {
	CreateChair() Chair
	CreateTable() Table
}

// 在classic_furniture_factory.go文件中实现了传统风格家具工厂类
// 在modern_furniture_factory.go文件中实现了现代风格家具工厂类
func main() {
	//创建现代风格工厂
	fmt.Println("创建现代风格的工厂，并演示创建现代桌子和现代椅子:")
	modernFactory := &ModernFurnitureFactory{}
	mChair := modernFactory.CreateChair()
	mChair.SitOn()
	fmt.Printf("椅子的风格：%s\n\n", mChair.GetStyle())

	mTable := modernFactory.CreateTable()
	mTable.PutOn()
	fmt.Printf("桌子的风格：%s\n", mTable.GetStyle())

	//创建经典风格工厂
	fmt.Println("\n创建传统风格的工厂，并演示创建传统桌子和传统椅子:")
	classicFactory := &ClassicFurnitureFactory{}
	cChair := classicFactory.CreateChair()
	cChair.SitOn()
	fmt.Printf("椅子的风格：%s\n\n", cChair.GetStyle())

	cTable := classicFactory.CreateTable()
	cTable.PutOn()
	fmt.Printf("桌子的风格：%s\n", cTable.GetStyle())
}
