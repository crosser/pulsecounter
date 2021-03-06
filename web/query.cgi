#!/usr/bin/env runhaskell

--                                                                        --
-- I am truly sorry. I would have used servant, but it failed to install. --
-- I would have used sqeletto, but got thrown back by depencencies.       --
-- I would have used some standard config file parser but they are all    --
-- overkills. This program is a very quick and dirty hack.                --
--                                                                        --

{-# LANGUAGE OverloadedStrings #-}

module Main where

import Control.Monad
import Data.Maybe
import Data.List
import Data.Ratio
import System.Locale
import System.Time
import Network.CGI
import Database.MySQL.Simple

main = runCGI $ handleErrors cgiMain

cgiMain :: CGI CGIResult
cgiMain = do
  conf <- liftIO $ readConf "/etc/watermeter.db"
  conn <- liftIO $ connect defaultConnectInfo { connectHost = host conf
					      , connectUser = user conf
                                              , connectPassword = pass conf
                                              , connectDatabase = dbnm conf
                                              }
  _ <- liftIO $ execute_ conn "set time_zone = '+00:00';";
  today <- liftIO getClockTime
  let tomorrow = addToClockTime (noTimeDiff {tdDay = 1}) today
      daystart x = (toUTCTime x) { ctHour = 0, ctMin = 0
                                 , ctSec = 0, ctPicosec = 0}
      dtstr x = formatCalendarTime defaultTimeLocale "%Y-%m-%d %H:%M:%S" x
      dlo = dtstr $ daystart today
      dhi = dtstr $ daystart tomorrow
  ilo <- getInput "lo"
  ihi <- getInput "hi"
  -- _ <- liftIO $ putStrLn $ " dlo=" ++ show dlo ++ " ilo=" ++ show ilo
  --                      ++ " dhi=" ++ show dhi ++ " ihi=" ++ show ihi
  let slo = fromMaybe dlo ilo :: String
      shi = fromMaybe dhi ihi :: String
  [(olo, ohi)] <- liftIO $ query conn "select unix_timestamp(?), unix_timestamp(?);"
                         [slo, shi]
  cold <- liftIO $ query conn
    "select unix_timestamp(timestamp) as time, value+adj as value from \
       \(select c.timestamp timestamp, c.value value, \
         \(select sum(value) from coldadj a where a.timestamp <= c.timestamp \
         \) adj from coldcnt c where timestamp between ? and ? \
       \) t order by timestamp;" (slo, shi)
  hot <- liftIO $ query conn
    "select unix_timestamp(timestamp) as time, value+adj as value from \
       \(select c.timestamp timestamp, c.value value, \
         \(select sum(value) from hotadj a where a.timestamp <= c.timestamp \
         \) adj from hotcnt c where timestamp between ? and ? \
       \) t order by timestamp;" (slo, shi)
  [(ccold, chot)] <- liftIO $ query_ conn
    "select lcold+acold as cold, lhot+ahot as hot from \
    \(select value as lcold from coldcnt order by timestamp desc limit 1) cc, \
    \(select sum(value) as acold from coldadj) ac, \
    \(select value as lhot from hotcnt order by timestamp desc limit 1) ch, \
    \(select sum(value) as ahot from hotadj) ah;"
  _ <- liftIO $ close conn

  setHeader "Content-type" "application/json"
  output $ "{\"range\": {\"lo\": " ++ show (floor (olo :: (Ratio Integer)))
          ++ ", \"hi\": " ++ show (floor (ohi :: (Ratio Integer)))
          ++ "}, \"current\": {\"cold\": " ++ show (floor (ccold :: (Ratio Integer)))
          ++ ", \"hot\": " ++ show (floor (chot :: (Ratio Integer)))
          ++ "}, \"cold\": [" ++ showjson cold
          ++ "], \"hot\": [" ++ showjson hot ++ "]}\n"

showjson :: [(Int, (Ratio Integer))] -> String
showjson l = intercalate "," $ map (\(t, c) -> "[" ++ show t ++ "," ++ show (floor c) ++ "]") l

data Conf = Conf { host :: String
		 , user :: String
		 , pass :: String
		 , dbnm :: String
		 }

readConf :: String -> IO Conf
readConf fn =
  readFile fn >>= return . (foldr parseLine (Conf "" "" "" "")) . lines
  where
    parseLine :: String -> Conf -> Conf
    parseLine l sum =
      case words l of
        [k, v] ->
          case k of
            "host" -> sum { host = v }
            "user" -> sum { user = v }
            "password" -> sum { pass = v }
            "database" -> sum { dbnm = v }
            _ -> error $ "bad key in config line \"" ++ l ++ "\""
        _ -> error $ "bad config line \"" ++ l ++ "\""
