package main

import (
	"encoding/json"
	"flag"
	"io/ioutil"
	"log"
	"strings"
)

type GameDataConfig struct {
	PkgName string `json:"pkg_name"`
	Member []struct {
		Attr               string `json:"attr"`
		Type               string `json:"type"`
		NeedProfile        bool   `json:"need_profile,omitempty"`
		InnerList          string `json:"inner_list,omitempty"`
		InnerListItemType  string `json:"inner_list_item_type,omitempty"`
		InnerListItemIdKey string `json:"inner_list_item_id_key,omitempty"`
	} `json:"member"`
}

var dataConfigFile string
var targetPath string
var gameDataConfig GameDataConfig
var pkgName string
var pkgNameCapFirst string

func main() {
	flag.StringVar(&dataConfigFile, "c", "gameData.json", "game data config file")
	flag.StringVar(&targetPath, "o", "./", "target path")

	flag.Parse()

	file, err := ioutil.ReadFile(dataConfigFile)
	if err != nil {
		log.Fatalf("Some error occured while reading file. Error: %s", err.Error())
	}

	err = json.Unmarshal(file, &gameDataConfig)
	if err != nil {
		log.Fatalf("Error occured during unmarshaling. Error: %s", err.Error())
	}

	if gameDataConfig.PkgName == "" {
		log.Fatalf("pkg_name cant be empty")
	}

	pkgName = gameDataConfig.PkgName
	pkgNameCapFirst = strings.Title(pkgName)

	//fmt.Printf("game data config: %#v\n", gameDataConfig)
	genUtils()
	genGameObject()
	genRecordObject()
	genRoleBuilder()
}
