namespace System
{
    // Minimal polyfill for C# 8 Index / Range on .NET Framework
    internal readonly struct Index
    {
        private readonly int _value;
        private readonly bool _fromEnd;

        public Index(int value, bool fromEnd = false)
        {
            if (value < 0)
                throw new ArgumentOutOfRangeException(nameof(value));
            _value = value;
            _fromEnd = fromEnd;
        }

        public int Value => _value;
        public bool FromEnd => _fromEnd;

        public static implicit operator Index(int value) => new Index(value);
        public override string ToString() => (_fromEnd ? "^" : "") + _value;
    }

    internal readonly struct Range
    {
        public Index Start { get; }
        public Index End { get; }

        public Range(Index start, Index end)
        {
            Start = start;
            End = end;
        }

        public static Range StartAt(Index start) => new Range(start, new Index(0, true));
        public static Range EndAt(Index end) => new Range(new Index(0), end);
        public static Range All => new Range(new Index(0), new Index(0, true));
        public override string ToString() => $"{Start}..{End}";
    }
}
