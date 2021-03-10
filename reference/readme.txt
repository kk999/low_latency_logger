目標：
	* 低的寫日誌動作時耗。
	* 相容於tradetron的日誌nqlog(C:\Job\TradeTron\project\nqlog)。

行為：
	寫日誌時會先把資料寫入記憶體中，再由另一個執行緒把資料由記憶體中取出來寫到檔案中。

Logger.hpp
	主要的並且是唯一的檔案。

main.cpp
	使用範例檔。
