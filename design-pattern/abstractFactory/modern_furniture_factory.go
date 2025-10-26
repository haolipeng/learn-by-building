package main

import "fmt"

// 现代风格的椅子
type ModernChair struct {
}

func (m *ModernChair) SitOn() {
	fmt.Printf("坐在现代风格的椅子上\n")
}

func (m *ModernChair) GetStyle() string {
	return "现代风格"
}

// 现代风格的桌子
type ModernTable struct {
}

func (m *ModernTable) PutOn() {
	fmt.Printf("放在现代风格的桌子上\n")
}

func (m *ModernTable) GetStyle() string {
	return "现代风格"
}

// 现代风格家具工厂
type ModernFurnitureFactory struct{}

func (m *ModernFurnitureFactory) CreateChair() Chair {
	return &ModernChair{}
}

func (m *ModernFurnitureFactory) CreateTable() Table {
	return &ModernTable{}
}
