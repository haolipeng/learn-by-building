package main

import "fmt"

// 传统风格的椅子
type ClassicChair struct {
}

func (m *ClassicChair) SitOn() {
	fmt.Printf("坐在传统风格的椅子上\n")
}

func (m *ClassicChair) GetStyle() string {
	return "传统风格"
}

// 传统风格的桌子
type ClassicTable struct {
}

func (m *ClassicTable) PutOn() {
	fmt.Printf("放在传统风格的桌子上\n")
}

func (m *ClassicTable) GetStyle() string {
	return "传统风格"
}

// 传统风格家具工厂
type ClassicFurnitureFactory struct{}

func (c *ClassicFurnitureFactory) CreateChair() Chair {
	return &ClassicChair{}
}

func (c *ClassicFurnitureFactory) CreateTable() Table {
	return &ClassicTable{}
}
