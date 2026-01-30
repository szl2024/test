package main

import (
	"flag"
	"fmt"
	"os"
	"path/filepath"

	"FCU_Tools/M1"
	"FCU_Tools/M1/M1_Public_Data"
	"FCU_Tools/M2"
	"FCU_Tools/M3"
	"FCU_Tools/M4"
	"FCU_Tools/M5"
	"FCU_Tools/M6"
	"FCU_Tools/Public_data"
	"FCU_Tools/SWC_Dependence"
)

func main() {
	connectorDir := flag.String("connector-dir", "", "input directory containing asw.csv")
	modelDir := flag.String("model-dir", "", "model directory for M1 analysis")
	quiet := flag.Bool("quiet", false, "print only final output path")
	flag.Parse()

	outputWriter := os.Stdout
	if *quiet {
		devNull, err := os.OpenFile(os.DevNull, os.O_WRONLY, 0)
		if err == nil {
			os.Stdout = devNull
			defer devNull.Close()
		}
	}

	if *connectorDir == "" {
		fmt.Fprintln(os.Stderr, "connector-dir is required")
		os.Exit(1)
	}
	if *modelDir == "" {
		fmt.Fprintln(os.Stderr, "model-dir is required")
		os.Exit(1)
	}
	if err := ensureDirExists(*modelDir); err != nil {
		fmt.Fprintln(os.Stderr, "model-dir error:", err)
		os.Exit(1)
	}

	if err := Public_data.InitOutputDirectoryWithConnectorDir(*connectorDir); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	printProgress(outputWriter, 10)

	SWC_Dependence.AnalyzeSWCDependencies(Public_data.ConnectorFilePath)
	printProgress(outputWriter, 20)

	M1_Public_Data.SrcPath = *modelDir
	M1main.M1_main()
	printProgress(outputWriter, 40)
	M2main.M2_main()
	printProgress(outputWriter, 55)
	M3main.M3_main()
	printProgress(outputWriter, 70)
	M4main.M4_main()
	printProgress(outputWriter, 80)
	M5main.M5_main()
	printProgress(outputWriter, 90)
	M6main.M6_main()
	printProgress(outputWriter, 100)

	outputPath := filepath.Join(Public_data.OutputDir, "result.ldi.xml")
	fprintln(outputWriter, outputPath)
}

func ensureDirExists(path string) error {
	info, err := os.Stat(path)
	if err != nil {
		return err
	}
	if !info.IsDir() {
		return fmt.Errorf("not a directory: %s", path)
	}
	return nil
}

func printProgress(w *os.File, percent int) {
	fprintln(w, fmt.Sprintf("PROGRESS:%d", percent))
}

func fprintln(w *os.File, text string) {
	if w == nil {
		return
	}
	_, _ = fmt.Fprintln(w, text)
}
